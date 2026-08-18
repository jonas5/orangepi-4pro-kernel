// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Allwinner sun60i PCIe PHY driver
 *
 * Configures SerDes PHY lane for PCIe 3.0 x1 on A733.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

/* PCIe PHY register offsets (mapped via SerDes) */
#define PCIE_PHY_CTRL		0x0200
#define PCIE_PHY_STATUS		0x0204
#define PCIE_PHY_TRIM		0x0208
#define PCIE_PHY_EQ		0x020c
#define PCIE_PHY_TIMER		0x0210

/* Control bits */
#define PCIE_CTRL_LANE_MASK	GENMASK(1, 0)
#define PCIE_CTRL_MODE_RC	0x00	/* Root Complex */
#define PCIE_CTRL_MODE_EP	0x01	/* Endpoint */
#define PCIE_CTRL_SSC_EN	BIT(4)
#define PCIE_CTRL_POWER_ON	BIT(8)
#define PCIE_CTRL_LANE0_EN	BIT(12)
#define PCIE_CTRL_LANE1_EN	BIT(13)
#define PCIE_CTRL_CLKREQ	BIT(16)

/* Status bits */
#define PCIE_STATUS_PLL_LOCK	BIT(0)
#define PCIE_STATUS_PIPE_READY	BIT(1)
#define PCIE_STATUS_RX_EQ_DONE	BIT(2)

/* Equalization */
#define PCIE_EQ_TX_PRESET_MASK	GENMASK(3, 0)
#define PCIE_EQ_TX_PRESET(x)	((x) & PCIE_EQ_TX_PRESET_MASK)
#define PCIE_EQ_RX_PRESET_MASK	GENMASK(7, 4)
#define PCIE_EQ_RX_PRESET(x)	(((x) & 0xf) << 4)

#define PCIE_PHY_TIMEOUT_US	10000

struct sun60i_pcie_phy {
	struct device		*dev;
	struct phy		*serdes;
	struct clk		*clk;
	struct clk		*pipe_clk;
	struct regulator	*supply;
	enum phy_mode		mode;
	int			lane;	/* 0 or 1 */
	void __iomem		*base;
};

static int sun60i_pcie_phy_power_on(struct phy *phy)
{
	struct sun60i_pcie_phy *priv = phy_get_drvdata(phy);
	u32 val;
	int ret;

	if (priv->supply) {
		ret = regulator_enable(priv->supply);
		if (ret)
			return ret;
	}

	ret = clk_prepare_enable(priv->clk);
	if (ret) {
		if (priv->supply)
			regulator_disable(priv->supply);
		return ret;
	}

	/* Configure SerDes lane for PCIe mode */
	if (priv->serdes) {
		ret = phy_set_mode_ext(priv->serdes, PHY_MODE_PCIE, 0);
		if (ret) {
			dev_err(priv->dev, "Failed to set SerDes mode: %d\n",
				ret);
			goto err_disable_clk;
		}
	}

	/* Configure PCIe PHY */
	val = PCIE_CTRL_SSC_EN | PCIE_CTRL_POWER_ON | PCIE_CTRL_CLKREQ;

	if (priv->mode == PHY_MODE_PCIE) {
		val |= PCIE_CTRL_MODE_EP;
	} else {
		val |= PCIE_CTRL_MODE_RC;
	}

	/* Enable appropriate lane */
	if (priv->lane == 0)
		val |= PCIE_CTRL_LANE0_EN;
	else
		val |= PCIE_CTRL_LANE1_EN;

	writel(val, priv->base + PCIE_PHY_CTRL);

	/* Configure equalization for PCIe 3.0 */
	writel(PCIE_EQ_TX_PRESET(0x02) | PCIE_EQ_RX_PRESET(0x02),
	       priv->base + PCIE_PHY_EQ);

	/* Wait for PLL lock and PIPE ready */
	ret = readl_poll_timeout(priv->base + PCIE_PHY_STATUS, val,
				 (val & PCIE_STATUS_PLL_LOCK) &&
				 (val & PCIE_STATUS_PIPE_READY),
				 100, PCIE_PHY_TIMEOUT_US);
	if (ret) {
		dev_err(priv->dev, "PCIe PHY not ready\n");
		goto err_disable_clk;
	}

	return 0;

err_disable_clk:
	clk_disable_unprepare(priv->clk);
	if (priv->supply)
		regulator_disable(priv->supply);
	return ret;
}

static int sun60i_pcie_phy_power_off(struct phy *phy)
{
	struct sun60i_pcie_phy *priv = phy_get_drvdata(phy);

	writel(0, priv->base + PCIE_PHY_CTRL);

	clk_disable_unprepare(priv->clk);

	if (priv->supply)
		regulator_disable(priv->supply);

	return 0;
}

static int sun60i_pcie_phy_set_mode(struct phy *phy, enum phy_mode mode,
				    int submode)
{
	struct sun60i_pcie_phy *priv = phy_get_drvdata(phy);

	switch (mode) {
	case PHY_MODE_PCIE:
	case PHY_MODE_PCIE_A:
	case PHY_MODE_PCIE_B:
		priv->mode = mode;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct phy_ops sun60i_pcie_phy_ops = {
	.power_on	= sun60i_pcie_phy_power_on,
	.power_off	= sun60i_pcie_phy_power_off,
	.set_mode	= sun60i_pcie_phy_set_mode,
	.owner		= THIS_MODULE,
};

static int sun60i_pcie_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sun60i_pcie_phy *priv;
	struct phy_provider *provider;
	struct phy *phy;
	struct resource *res;
	u32 lane;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) {
		priv->base = devm_ioremap_resource(dev, res);
		if (IS_ERR(priv->base))
			return PTR_ERR(priv->base);
	} else {
		return dev_err_probe(dev, -ENODEV,
				     "No memory resource found\n");
	}

	priv->clk = devm_clk_get_optional(dev, "phy_clk");
	if (IS_ERR(priv->clk))
		return dev_err_probe(dev, PTR_ERR(priv->clk),
				     "Failed to get phy_clk\n");

	priv->pipe_clk = devm_clk_get_optional(dev, "pipe_clk");
	if (IS_ERR(priv->pipe_clk))
		return dev_err_probe(dev, PTR_ERR(priv->pipe_clk),
				     "Failed to get pipe_clk\n");

	priv->supply = devm_regulator_get_optional(dev, "vpcie");
	if (IS_ERR(priv->supply)) {
		if (PTR_ERR(priv->supply) != -ENODEV)
			return dev_err_probe(dev, PTR_ERR(priv->supply),
					     "Failed to get vpcie supply\n");
		priv->supply = NULL;
	}

	priv->serdes = devm_phy_optional_get(dev, "serdes");
	if (IS_ERR(priv->serdes))
		return dev_err_probe(dev, PTR_ERR(priv->serdes),
				     "Failed to get SerDes PHY\n");

	if (of_property_read_u32(dev->of_node, "allwinner,lane", &lane))
		lane = 0;

	if (lane >= 2) {
		dev_err(dev, "Invalid lane %u (must be 0 or 1)\n", lane);
		return -EINVAL;
	}

	priv->lane = lane;
	priv->mode = PHY_MODE_PCIE;

	platform_set_drvdata(pdev, priv);

	phy = devm_phy_create(dev, NULL, &sun60i_pcie_phy_ops);
	if (IS_ERR(phy))
		return dev_err_probe(dev, PTR_ERR(phy),
				     "Failed to create PCIe PHY\n");

	phy_set_drvdata(phy, priv);

	provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(provider))
		return dev_err_probe(dev, PTR_ERR(provider),
				     "Failed to register PHY provider\n");

	return 0;
}

static const struct of_device_id sun60i_pcie_phy_of_match[] = {
	{ .compatible = "allwinner,sun60i-a733-pcie-phy" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, sun60i_pcie_phy_of_match);

static struct platform_driver sun60i_pcie_phy_driver = {
	.probe	= sun60i_pcie_phy_probe,
	.driver	= {
		.name		= "sun60i-pcie-phy",
		.of_match_table	= sun60i_pcie_phy_of_match,
	},
};
module_platform_driver(sun60i_pcie_phy_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allwinner");
MODULE_DESCRIPTION("Allwinner sun60i PCIe PHY driver for A733");
