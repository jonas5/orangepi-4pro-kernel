// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Allwinner sun60i GMAC Ethernet driver
 *
 * Based on the sun8i-emac driver and Synopsys DesignWare MAC documentation.
 * The A733 GMAC is a DesignWare MAC with 4 DMA queues and RGMII support.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/phy.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/stmmac.h>

#define GMAC_SUN60I_NAME	"sun60i-gmac"

/* GMAC register offsets */
#define GMAC_TX_CTRL		0x0000
#define GMAC_RX_CTRL		0x0004
#define GMAC_TX_DESC_LIST	0x0100
#define GMAC_RX_DESC_LIST	0x0104
#define GMAC_TX_DESC_CTRL	0x0108
#define GMAC_RX_DESC_CTRL	0x010c
#define GMAC_TX_DESC_RING	0x0110
#define GMAC_RX_DESC_RING	0x0114
#define GMAC_TX_STATUS		0x0120
#define GMAC_RX_STATUS		0x0124
#define GMAC_INT_STATUS		0x0130
#define GMAC_INT_ENABLE		0x0134
#define GMAC_RGMII_STATUS	0x0138
#define GMAC_MDIO_ADDR		0x0200
#define GMAC_MDIO_DATA		0x0204
#define GMAC_VLAN_TAG		0x0280
#define GMAC_VLAN_FILTER	0x0284

/* TX control register bits */
#define GMAC_TX_ENABLE		BIT(0)
#define GMAC_TX_CR		BIT(1)		/* CRC stripping */

/* RX control register bits */
#define GMAC_RX_ENABLE		BIT(0)
#define GMAC_RX_ACS		BIT(1)		/* auto checksum */

/* Interrupt bits */
#define GMAC_INT_TX		BIT(0)
#define GMAC_INT_RX		BIT(2)
#define GMAC_INT_ABNORMAL	BIT(14)
#define GMAC_INT_NORMAL		BIT(15)
#define GMAC_INT_RX_TIMER	BIT(6)
#define GMAC_INT_TX_COMPLETE	BIT(0)

/* RGMII status register */
#define GMAC_RGMII_SPEED_MASK	GENMASK(1, 0)
#define GMAC_RGMII_SPEED_1000	0x02
#define GMAC_RGMII_SPEED_100	0x01
#define GMAC_RGMII_SPEED_10	0x00

/* TX/RX descriptor sizes */
#define NUM_TX_DESCS		256
#define NUM_RX_DESCS		256
#define DESC_BUF_LEN		PAGE_SIZE

/* DMA descriptors */
struct sun60i_dma_desc {
	__le32 status;
	__le32 buffer;
	__le32 next;
	__le32 ext_status;
};

/* RX buffer descriptor status bits */
#define RX_DESC_OWN		BIT(31)
#define RX_DESC_A0		BIT(30)
#define RX_DESC_A1		BIT(29)
#define RX_DESC_FS		BIT(9)
#define RX_DESC_LS		BIT(8)

/* TX buffer descriptor status bits */
#define TX_DESC_OWN		BIT(31)
#define TX_DESC_FS		BIT(29)
#define TX_DESC_LS		BIT(28)
#define TX_DESC_LEN_MASK	GENMASK(10, 0)

struct sun60i_eth_stats {
	u32 tx_packets;
	u32 rx_packets;
	u32 tx_errors;
	u32 rx_errors;
};

struct sun60i_eth {
	struct device		*dev;
	struct net_device	*ndev;
	void __iomem		*base;
	int			irq;

	struct clk		*tx_clk;
	struct clk		*rx_clk;
	struct clk		*mac_clk;

	struct phy_device	*phydev;
	struct phy		*serdes;
	int			speed;
	int			duplex;

	/* DMA descriptors */
	struct sun60i_dma_desc	*tx_dma;
	struct sun60i_dma_desc	*rx_dma;
	dma_addr_t		tx_dma_phys;
	dma_addr_t		rx_dma_phys;

	/* TX/RX ring indices */
	unsigned int		tx_head;
	unsigned int		tx_tail;
	unsigned int		rx_head;
	unsigned int		rx_tail;

	/* NAPI */
	struct napi_struct	napi;
	struct napi_struct	tx_napi;

	/* Stats */
	struct rtnl_link_stats64 stats;

	/* Configuration */
	bool			use_internal_delay;
};

static void sun60i_eth_mac_enable(void __iomem *base, bool enable)
{
	u32 val;

	val = readl(base + GMAC_TX_CTRL);
	if (enable)
		val |= GMAC_TX_ENABLE;
	else
		val &= ~GMAC_TX_ENABLE;
	writel(val, base + GMAC_TX_CTRL);

	val = readl(base + GMAC_RX_CTRL);
	if (enable)
		val |= GMAC_RX_ENABLE | GMAC_RX_ACS;
	else
		val &= ~(GMAC_RX_ENABLE | GMAC_RX_ACS);
	writel(val, base + GMAC_RX_CTRL);
}

static void sun60i_eth_set_mac_addr(void __iomem *base, const u8 *addr)
{
	u32 hi, lo;

	lo = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
	hi = (addr[5] << 8) | addr[4];

	writel(lo, base + 0x40);
	writel(hi, base + 0x44);
}

static int sun60i_eth_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	struct sun60i_eth *priv = bus->priv;
	u32 val;
	int ret;

	if (phyreg & MII_DEVADDR_C45)
		return -EOPNOTSUPP;

	val = MII_BUSY | MII_CLKRANGE_150_250 | (phyaddr << 11) |
	      (phyreg << 6);
	writel(val, priv->base + GMAC_MDIO_ADDR);

	ret = readl_poll_timeout(priv->base + GMAC_MDIO_ADDR, val,
				 !(val & MII_BUSY), 100, 10000);
	if (ret)
		return -ETIMEDOUT;

	return readl(priv->base + GMAC_MDIO_DATA);
}

static int sun60i_eth_mdio_write(struct mii_bus *bus, int phyaddr,
				 int phyreg, u16 phydata)
{
	struct sun60i_eth *priv = bus->priv;
	u32 val;
	int ret;

	if (phyreg & MII_DEVADDR_C45)
		return -EOPNOTSUPP;

	writel(phydata, priv->base + GMAC_MDIO_DATA);

	val = MII_BUSY | MII_WRITE | MII_CLKRANGE_150_250 | (phyaddr << 11) |
	      (phyreg << 6);
	writel(val, priv->base + GMAC_MDIO_ADDR);

	ret = readl_poll_timeout(priv->base + GMAC_MDIO_ADDR, val,
				 !(val & MII_BUSY), 100, 10000);
	if (ret)
		return -ETIMEDOUT;

	return 0;
}

static int sun60i_eth_mdio_init(struct sun60i_eth *priv)
{
	struct device_node *np = priv->dev->of_node;
	struct mii_bus *bus;

	bus = mdiobus_alloc();
	if (!bus)
		return -ENOMEM;

	bus->name = GMAC_SUN60I_NAME;
	bus->read = sun60i_eth_mdio_read;
	bus->write = sun60i_eth_mdio_write;
	bus->priv = priv;
	bus->parent = priv->dev;
	snprintf(bus->id, MII_BUS_ID_SIZE, "%pOF", np);

	if (bus->name[0] == '/')
		snprintf(bus->id, MII_BUS_ID_SIZE, "sun60i");

	priv->phydev = of_phy_connect(bus, np, NULL, 0,
					priv->use_internal_delay ?
					PHY_INTERFACE_MODE_RGMII_ID :
					PHY_INTERFACE_MODE_RGMII);
	if (IS_ERR(priv->phydev)) {
		dev_err(priv->dev, "Could not attach PHY (%ld)\n",
			PTR_ERR(priv->phydev));
		mdiobus_free(bus);
		return PTR_ERR(priv->phydev);
	}

	phy_set_max_speed(priv->phydev, 1000);
	priv->phydev->supported &= PHY_GBIT_FEATURES;
	priv->phydev->advertising = priv->phydev->supported;

	return mdiobus_register(bus);
}

static void sun60i_eth_init_dma_desc_ring(struct sun60i_eth *priv)
{
	int i;

	priv->tx_head = 0;
	priv->tx_tail = 0;
	priv->rx_head = 0;
	priv->rx_tail = 0;

	for (i = 0; i < NUM_TX_DESCS; i++) {
		priv->tx_dma[i].status = cpu_to_le32(0);
		priv->tx_dma[i].buffer = cpu_to_le32(0);
		priv->tx_dma[i].next = cpu_to_le32(0);
		priv->tx_dma[i].ext_status = cpu_to_le32(0);
	}

	for (i = 0; i < NUM_RX_DESCS; i++) {
		priv->rx_dma[i].status = cpu_to_le32(RX_DESC_OWN);
		priv->rx_dma[i].buffer = cpu_to_le32(0);
		priv->rx_dma[i].next = cpu_to_le32(0);
		priv->rx_dma[i].ext_status = cpu_to_le32(0);
	}

	/* Chain TX descriptors */
	for (i = 0; i < NUM_TX_DESCS - 1; i++)
		priv->tx_dma[i].next = cpu_to_le32(priv->tx_dma_phys +
					(i + 1) * sizeof(struct sun60i_dma_desc));

	priv->tx_dma[NUM_TX_DESCS - 1].next = cpu_to_le32(priv->tx_dma_phys);

	/* Chain RX descriptors */
	for (i = 0; i < NUM_RX_DESCS - 1; i++)
		priv->rx_dma[i].next = cpu_to_le32(priv->rx_dma_phys +
					(i + 1) * sizeof(struct sun60i_dma_desc));

	priv->rx_dma[NUM_RX_DESCS - 1].next = cpu_to_le32(priv->rx_dma_phys);
}

static int sun60i_eth_init_dma(struct sun60i_eth *priv)
{
	struct device *dev = priv->dev;
	int size;

	/* Allocate TX DMA descriptors */
	size = NUM_TX_DESCS * sizeof(struct sun60i_dma_desc);
	priv->tx_dma = dma_alloc_coherent(dev, size, &priv->tx_dma_phys,
					  GFP_KERNEL);
	if (!priv->tx_dma)
		return -ENOMEM;

	/* Allocate RX DMA descriptors */
	size = NUM_RX_DESCS * sizeof(struct sun60i_dma_desc);
	priv->rx_dma = dma_alloc_coherent(dev, size, &priv->rx_dma_phys,
					  GFP_KERNEL);
	if (!priv->rx_dma) {
		dma_free_coherent(dev, NUM_TX_DESCS * sizeof(*priv->tx_dma),
				  priv->tx_dma, priv->tx_dma_phys);
		return -ENOMEM;
	}

	sun60i_eth_init_dma_desc_ring(priv);

	/* Set descriptor base addresses */
	writel((u32)upper_32_bits(priv->tx_dma_phys),
	       priv->base + GMAC_TX_DESC_RING);
	writel((u32)priv->tx_dma_phys,
	       priv->base + GMAC_TX_DESC_LIST);

	writel((u32)upper_32_bits(priv->rx_dma_phys),
	       priv->base + GMAC_RX_DESC_RING);
	writel((u32)priv->rx_dma_phys,
	       priv->base + GMAC_RX_DESC_LIST);

	/* Set descriptor length (number of descriptors) */
	writel(NUM_TX_DESCS, priv->base + GMAC_TX_DESC_CTRL);
	writel(NUM_RX_DESCS, priv->base + GMAC_RX_DESC_CTRL);

	return 0;
}

static void sun60i_eth_free_dma(struct sun60i_eth *priv)
{
	struct device *dev = priv->dev;
	int size;

	if (priv->rx_dma) {
		size = NUM_RX_DESCS * sizeof(*priv->rx_dma);
		dma_free_coherent(dev, size, priv->rx_dma,
				  priv->rx_dma_phys);
		priv->rx_dma = NULL;
	}

	if (priv->tx_dma) {
		size = NUM_TX_DESCS * sizeof(*priv->tx_dma);
		dma_free_coherent(dev, size, priv->tx_dma,
				  priv->tx_dma_phys);
		priv->tx_dma = NULL;
	}
}

static void sun60i_eth_update_speed(struct sun60i_eth *priv)
{
	u32 val;

	val = readl(priv->base + GMAC_RGMII_STATUS);
	val &= ~GMAC_RGMII_SPEED_MASK;

	switch (priv->speed) {
	case 1000:
		val |= GMAC_RGMII_SPEED_1000;
		break;
	case 100:
		val |= GMAC_RGMII_SPEED_100;
		break;
	default:
		val |= GMAC_RGMII_SPEED_10;
		break;
	}

	writel(val, priv->base + GMAC_RGMII_STATUS);
}

static int sun60i_eth_open(struct net_device *ndev)
{
	struct sun60i_eth *priv = netdev_priv(ndev);
	int ret;

	ret = sun60i_eth_init_dma(priv);
	if (ret) {
		dev_err(priv->dev, "Failed to init DMA: %d\n", ret);
		return ret;
	}

	sun60i_eth_set_mac_addr(priv->base, ndev->dev_addr);
	sun60i_eth_mac_enable(priv->base, true);

	priv->speed = priv->phydev->speed;
	priv->duplex = priv->phydev->duplex;
	sun60i_eth_update_speed(priv);

	ret = request_irq(priv->irq, stmmac_interrupt, 0, ndev->name, priv);
	if (ret) {
		dev_err(priv->dev, "Failed to request IRQ %d: %d\n",
			priv->irq, ret);
		sun60i_eth_free_dma(priv);
		return ret;
	}

	napi_enable(&priv->napi);
	phy_start(priv->phydev);
	netif_start_queue(ndev);

	/* Enable interrupts */
	writel(GMAC_INT_TX | GMAC_INT_RX | GMAC_INT_ABNORMAL |
	       GMAC_INT_NORMAL, priv->base + GMAC_INT_ENABLE);

	return 0;
}

static int sun60i_eth_stop(struct net_device *ndev)
{
	struct sun60i_eth *priv = netdev_priv(ndev);

	netif_stop_queue(ndev);
	napi_disable(&priv->napi);
	phy_stop(priv->phydev);
	sun60i_eth_mac_enable(priv->base, false);
	free_irq(priv->irq, priv);
	sun60i_eth_free_dma(priv);

	return 0;
}

static netdev_tx_t sun60i_eth_xmit(struct sk_buff *skb,
				    struct net_device *ndev)
{
	struct sun60i_eth *priv = netdev_priv(ndev);
	struct sun60i_dma_desc *desc;
	dma_addr_t dma;
	unsigned int entry;
	int len;

	if (!priv->tx_dma) {
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}

	entry = priv->tx_head;
	desc = &priv->tx_dma[entry];

	if (le32_to_cpu(desc->status) & TX_DESC_OWN) {
		netif_stop_queue(ndev);
		return NETDEV_TX_BUSY;
	}

	len = skb->len;
	dma = dma_map_single(priv->dev, skb->data, len, DMA_TO_DEVICE);
	if (dma_mapping_error(priv->dev, dma)) {
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}

	desc->buffer = cpu_to_le32(dma);
	desc->ext_status = cpu_to_le32(0);
	desc->status = cpu_to_le32(TX_DESC_FS | TX_DESC_LS |
				   (len & TX_DESC_LEN_MASK) | TX_DESC_OWN);

	priv->tx_head = (entry + 1) % NUM_TX_DESCS;

	skb_tx_timestamp(skb);

	/* Transmit */
	writel(0, priv->base + GMAC_TX_DESC_CTRL);

	return NETDEV_TX_OK;
}

static int sun60i_eth_poll(struct napi_struct *napi, int budget)
{
	struct sun60i_eth *priv = container_of(napi, struct sun60i_eth, napi);
	struct net_device *ndev = priv->ndev;
	int work_done = 0;

	while (work_done < budget) {
		struct sun60i_dma_desc *desc;
		struct sk_buff *skb;
		unsigned int entry;
		int len;
		dma_addr_t dma;

		entry = priv->rx_tail;
		desc = &priv->rx_dma[entry];

		if (le32_to_cpu(desc->status) & RX_DESC_OWN)
			break;

		if (!(le32_to_cpu(desc->status) & RX_DESC_FS) ||
		    !(le32_to_cpu(desc->status) & RX_DESC_LS)) {
			priv->stats.rx_errors++;
			goto next;
		}

		len = le32_to_cpu(desc->status) & 0x3fff;
		skb = netdev_alloc_skb_ip_align(ndev, len);
		if (!skb) {
			priv->stats.rx_errors++;
			goto next;
		}

		dma = le32_to_cpu(desc->buffer);
		dma_sync_single_for_cpu(priv->dev, dma, len, DMA_FROM_DEVICE);
		skb_put_data(skb, phys_to_virt(dma), len);
		dma_sync_single_for_dev(priv->dev, dma, len, DMA_FROM_DEVICE);

		skb->protocol = eth_type_trans(skb, ndev);
		skb->ip_summed = CHECKSUM_UNNECESSARY;

		napi_gro_receive(&priv->napi, skb);
		priv->stats.rx_packets++;
		work_done++;

next:
		priv->rx_tail = (entry + 1) % NUM_RX_DESCS;
	}

	/* Re-arm RX descriptors */
	writel(NUM_RX_DESCS, priv->base + GMAC_RX_DESC_CTRL);

	if (work_done >= budget)
		return budget;

	napi_complete_done(&priv->napi, work_done);

	/* Re-enable RX interrupt */
ritel(GMAC_INT_RX | GMAC_INT_TX | GMAC_INT_ABNORMAL | GMAC_INT_NORMAL,
	      priv->base + GMAC_INT_ENABLE);

	return work_done;
}

static irqreturn_t stmmac_interrupt(int irq, void *dev_id)
{
	struct sun60i_eth *priv = dev_id;
	u32 status;

	status = readl(priv->base + GMAC_INT_STATUS);

	if (status & (GMAC_INT_RX | GMAC_INT_TX)) {
		/* Disable interrupts until napi re-enables them */
		writel(0, priv->base + GMAC_INT_ENABLE);
		napi_schedule(&priv->napi);
	}

	/* Clear interrupts */
	writel(status, priv->base + GMAC_INT_STATUS);

	return IRQ_HANDLED;
}

static void sun60i_eth_set_rx_mode(struct net_device *ndev)
{
	struct sun60i_eth *priv = netdev_priv(ndev);

	/* Basic multicast and promiscuous support */
	if (ndev->flags & IFF_PROMISC) {
		writel(0x00000001, priv->base + 0x40);
		writel(0x80000000, priv->base + 0x44);
	} else {
		/* Default MAC address filter */
		sun60i_eth_set_mac_addr(priv->base, ndev->dev_addr);
	}

	/* VLAN filtering */
	if (ndev->flags & IFF_ALLMULTI) {
		u32 val = readl(priv->base + GMAC_VLAN_FILTER);
		val |= BIT(0); /* Pass all multicast */
		writel(val, priv->base + GMAC_VLAN_FILTER);
	}
}

static const struct net_device_ops sun60i_eth_netdev_ops = {
	.ndo_open		= sun60i_eth_open,
	.ndo_stop		= sun60i_eth_stop,
	.ndo_start_xmit		= sun60i_eth_xmit,
	.ndo_set_rx_mode	= sun60i_eth_set_rx_mode,
	.ndo_get_stats64	= dev_get_tstats64,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_mac_address	= eth_mac_addr,
};

static void sun60i_eth_set_ethtool_ops(struct net_device *ndev)
{
	/* Minimal ethtool support via stmmac helpers */
}

static int sun60i_eth_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct sun60i_eth *priv;
	struct net_device *ndev;
	struct resource *res;
	const void *mac_addr;
	int ret;

	ndev = devm_alloc_etherdev(&pdev->dev, sizeof(*priv));
	if (!ndev)
		return -ENOMEM;

	priv = netdev_priv(ndev);
	priv->ndev = ndev;
	priv->dev = &pdev->dev;
	platform_set_drvdata(pdev, priv);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	priv->irq = platform_get_irq(pdev, 0);
	if (priv->irq < 0)
		return priv->irq;

	priv->tx_clk = devm_clk_get(&pdev->dev, "tx_clk");
	if (IS_ERR(priv->tx_clk))
		return dev_err_probe(&pdev->dev, PTR_ERR(priv->tx_clk),
				     "Failed to get tx_clk\n");

	priv->rx_clk = devm_clk_get(&pdev->dev, "rx_clk");
	if (IS_ERR(priv->rx_clk))
		return dev_err_probe(&pdev->dev, PTR_ERR(priv->rx_clk),
				     "Failed to get rx_clk\n");

	priv->mac_clk = devm_clk_get(&pdev->dev, "mac_clk");
	if (IS_ERR(priv->mac_clk))
		return dev_err_probe(&pdev->dev, PTR_ERR(priv->mac_clk),
				     "Failed to get mac_clk\n");

	ret = clk_prepare_enable(priv->tx_clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(priv->rx_clk);
	if (ret)
		goto err_disable_tx_clk;

	ret = clk_prepare_enable(priv->mac_clk);
	if (ret)
		goto err_disable_rx_clk;

	priv->use_internal_delay = of_property_read_bool(np,
					"allwinner,tx-delay-ps") ||
				   of_property_read_bool(np,
					"allwinner,rx-delay-ps");

	/* Get MAC address from DT or random */
	mac_addr = of_get_mac_address(np);
	if (IS_ERR(mac_addr)) {
		eth_hw_addr_random(ndev);
	} else {
		ether_addr_copy(ndev->dev_addr, mac_addr);
	}

	ndev->netdev_ops = &sun60i_eth_netdev_ops;
	ndev->min_mtu = ETH_ZLEN;
	ndev->max_mtu = 9000;
	ndev->features = NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |
			 NETIF_F_HW_VLAN_CTAG_FILTER |
			 NETIF_F_GSO_SOFTWARE | NETIF_F_SG;
	ndev->hw_features = ndev->features;

	sun60i_eth_set_ethtool_ops(ndev);

	INIT_NAPI(&priv->napi, sun60i_eth_poll, 64);

	ret = sun60i_eth_mdio_init(priv);
	if (ret) {
		dev_err(&pdev->dev, "Failed to init MDIO: %d\n", ret);
		goto err_disable_mac_clk;
	}

	ret = register_netdev(ndev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register netdev: %d\n", ret);
		goto err_disable_mac_clk;
	}

	return 0;

err_disable_mac_clk:
	clk_disable_unprepare(priv->mac_clk);
err_disable_rx_clk:
	clk_disable_unprepare(priv->rx_clk);
err_disable_tx_clk:
	clk_disable_unprepare(priv->tx_clk);
	return ret;
}

static int sun60i_eth_remove(struct platform_device *pdev)
{
	struct sun60i_eth *priv = platform_get_drvdata(pdev);
	struct net_device *ndev = priv->ndev;

	unregister_netdev(ndev);
	phy_disconnect(priv->phydev);
	mdiobus_unregister(priv->phydev->mdio.bus);
	mdiobus_free(priv->phydev->mdio.bus);

	clk_disable_unprepare(priv->mac_clk);
	clk_disable_unprepare(priv->rx_clk);
	clk_disable_unprepare(priv->tx_clk);

	return 0;
}

static const struct of_device_id sun60i_eth_of_match[] = {
	{ .compatible = "allwinner,sun60i-gmac" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, sun60i_eth_of_match);

static struct platform_driver sun60i_eth_driver = {
	.probe	= sun60i_eth_probe,
	.remove	= sun60i_eth_remove,
	.driver	= {
		.name		= GMAC_SUN60I_NAME,
		.of_match_table	= sun60i_eth_of_match,
	},
};
module_platform_driver(sun60i_eth_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allwinner");
MODULE_DESCRIPTION("Allwinner sun60i GMAC Ethernet driver");
