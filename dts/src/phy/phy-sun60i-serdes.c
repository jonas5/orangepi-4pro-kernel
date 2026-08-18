// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Allwinner sun60i SerDes PHY driver
 *
 * The A733 SerDes block multiplexes USB3, PCIe, and DisplayPort
 * across two configurable lanes.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

/* SerDes register offsets */
#define SERDES_LANECTRL0	0x0000
#define SERDES_LANECTRL1	0x0004
#define SERDES_PLLCTRL		0x0010
#define SERDES_STATE		0x0020
#define SERDES_POWERDOWN	0x0030
#define SERDES_RESET		0x0034
#define SERDES_RATE		0x0040
#define SERDES_TXCK_SEL		0x0050
#define SERDES_RX_EQ		0x0060
#define SERDES_TX_EMP		0x0070
#define SERDES_TEST		0x0080

/* Lane control register bits */
#define LANECTRL_MODE_MASK	GENMASK(3, 0)
#define LANECTRL_MODE_USB3	0x01
#define LANECTRL_MODE_PCIE	0x02
#define LANECTRL_MODE_DP	0x03
#define LANECTRL_MODE_SATA	0x04
#define LANECTRL_TX_ENABLE	BIT(8)
#define LANECTRL_RX_ENABLE	BIT(9)
#define LANECTRL_TX_RESET	BIT(16)
#define LANECTRL_RX_RESET	BIT(17)

/* PLL control bits */
#define PLLCTRL_POWER_ON	BIT(0)
#define PLLCTRL_DIV_MASK	GENMASK(7, 1)
#define PLLCTRL_DIV(x)		((x) << 1)
#define PLLCTRL_READY		BIT(8)

/* Power down bits */
#define POWERDOWN_LANE0		BIT(0)
#define POWERDOWN_LANE1		BIT(1)

/* Reset bits */
#define RESET_ALL		BIT(0)
#define RESET_LANE0		BIT(1)
#define RESET_LANE1		BIT(2)

/* Rate */
#define RATE_GEN1		0x00
#define RATE_GEN2		0x01
#define RATE_GEN3		0x02

#define SERDES_NUM_LANES	2
#define SERDES_TIMEOUT_US	10000

struct sun60i_serdes {
	void __iomem		*base;
	struct device		*dev;
	struct clk		*clk;
	struct clk		*bus_clk;
	struct regulator	*supply;
	bool			lane_in_use[SERDES_NUM_LANES];
	enum phy_mode		lane_mode[SERDES_NUM_LANES];
};

static u32 sun60i_serdes_read(void __iomem *base, u32 offset)
{
	return readl(base + offset);
}

static void sun60i_serdes_write(void __iomem *base, u32 offset, u32 val)
{
	writel(val, base + offset);
}

static void sun60i_serdes_reset(struct sun60i_serdes *serdes)
{
	u32 val;

	val = sun60i_serdes_read(serdes->base, SERDES_RESET);
	val |= RESET_ALL;
	sun60i_serdes_write(serdes->base, SERDES_RESET, val);
	usleep_range(10, 20);
	val &= ~RESET_ALL;
	sun60i_serdes_write(serdes->base, SERDES_RESET, val);
	usleep_range(100, 200);
}

static void sun60i_serdes_power_up(struct sun60i_serdes *serdes, int lane)
{
	u32 val;

	/* De-assert power down */
	val = sun60i_serdes_read(serdes->base, SERDES_POWERDOWN);
	if (lane == 0)
		val &= ~POWERDOWN_LANE0;
	else
		val &= ~POWERDOWN_LANE1;
	sun60i_serdes_write(serdes->base, SERDES_POWERDOWN, val);

	/* PLL power on */
	val = sun60i_serdes_read(serdes->base, SERDES_PLLCTRL);
	val |= PLLCTRL_POWER_ON;
	sun60i_serdes_write(serdes->base, SERDES_PLLCTRL, val);
	usleep_range(1000, 2000);
}

static void sun60i_serdes_power_down(struct sun60i_serdes *serdes, int lane)
{
	u32 val;

	val = sun60i_serdes_read(serdes->base, SERDES_POWERDOWN);
	if (lane == 0)
		val |= POWERDOWN_LANE0;
	else
		val |= POWERDOWN_LANE1;
	sun60i_serdes_write(serdes->base, SERDES_POWERDOWN, val);
}

static int sun60i_serdes_wait_ready(struct sun60i_serdes *serdes)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout(serdes->base + SERDES_PLLCTRL, val,
				 (val & PLLCTRL_READY), 100, SERDES_TIMEOUT_US);
	if (ret) {
		dev_err(serdes->dev, "SerDes PLL not ready\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int sun60i_serdes_configure_lane(struct sun60i_serdes *serdes,
					int lane, enum phy_mode mode)
{
	u32 val;
	u32 mode_val;
	u32 rate;

	switch (mode) {
	case PHY_MODE_USB_HOST_SS:
		mode_val = LANECTRL_MODE_USB3;
		rate = RATE_GEN3;
		break;
	case PHY_MODE_PCIE:
		mode_val = LANECTRL_MODE_PCIE;
		rate = RATE_GEN3;
		break;
	case PHY_MODE_DP:
		mode_val = LANECTRL_MODE_DP;
		rate = RATE_GEN2;
		break;
	default:
		dev_err(serdes->dev, "Unsupported mode %d for lane %d\n",
			mode, lane);
		return -EINVAL;
	}

	sun60i_serdes_power_up(serdes, lane);

	/* Set lane mode */
	val = sun60i_serdes_read(serdes->base, SERDES_LANECTRL0 + (lane * 4));
	val &= ~LANECTRL_MODE_MASK;
	val |= mode_val;
	val |= LANECTRL_TX_ENABLE | LANECTRL_RX_ENABLE;
	sun60i_serdes_write(serdes->base, SERDES_LANECTRL0 + (lane * 4), val);

	/* Set rate */
	val = sun60i_serdes_read(serdes->base, SERDES_RATE);
	val &= ~(GENMASK(3, 0) << (lane * 4));
	val |= (rate << (lane * 4));
	sun60i_serdes_write(serdes->base, SERDES_RATE, val);

	/* Configure TX equalization for PCIe */
	if (mode == PHY_MODE_PCIE) {
		val = sun60i_serdes_read(serdes->base, SERDES_TX_EMP);
		val |= (0x4 << (lane * 8));
		sun60i_serdes_write(serdes->base, SERDES_TX_EMP, val);
	}

	serdes->lane_mode[lane] = mode;
	serdes->lane_in_use[lane] = true;

	return 0;
}

static int sun60i_serdes_init(struct phy *phy)
{
	struct sun60i_serdes *serdes = phy_get_drvdata(phy);
	int ret;

	ret = clk_prepare_enable(serdes->clk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(serdes->bus_clk);
	if (ret) {
		clk_disable_unprepare(serdes->clk);
		return ret;
	}

	if (serdes->supply) {
		ret = regulator_enable(serdes->supply);
		if (ret) {
			clk_disable_unprepare(serdes->bus_clk);
			clk_disable_unprepare(serdes->clk);
			return ret;
		}
	}

	sun60i_serdes_reset(serdes);

	/* Power down all lanes initially */
	sun60i_serdes_power_down(serdes, 0);
	sun60i_serdes_power_down(serdes, 1);

	return 0;
}

static int sun60i_serdes_exit(struct phy *phy)
{
	struct sun60i_serdes *serdes = phy_get_drvdata(phy);

	sun60i_serdes_power_down(serdes, 0);
	sun60i_serdes_power_down(serdes, 1);

	if (serdes->supply)
		regulator_disable(serdes->supply);

	clk_disable_unprepare(serdes->bus_clk);
	clk_disable_unprepare(serdes->clk);

	return 0;
}

static int sun60i_serdes_configure(struct phy *phy, enum phy_mode mode,
				   int submode)
{
	struct sun60i_serdes *serdes = phy_get_drvdata(phy);
	int lane = phy->id;

	if (lane < 0 || lane >= SERDES_NUM_LANES)
		return -EINVAL;

	return sun60i_serdes_configure_lane(serdes, lane, mode);
}

static int sun60i_serdes_verify(struct phy *phy, enum phy_mode mode,
				int submode)
{
	struct sun60i_serdes *serdes = phy_get_drvdata(phy);
	int lane = phy->id;
	u32 val;

	val = sun60i_serdes_read(serdes->base, SERDES_LANECTRL0 + (lane * 4));
	val &= LANECTRL_MODE_MASK;

	switch (mode) {
	case PHY_MODE_USB_HOST_SS:
		return (val == LANECTRL_MODE_USB3) ? 0 : -EINVAL;
	case PHY_MODE_PCIE:
		return (val == LANECTRL_MODE_PCIE) ? 0 : -EINVAL;
	case PHY_MODE_DP:
		return (val == LANECTRL_MODE_DP) ? 0 : -EINVAL;
	default:
		return -EINVAL;
	}
}

static const struct phy_ops sun60i_serdes_ops = {
	.init		= sun60i_serdes_init,
	.exit		= sun60i_serdes_exit,
	.configure	= sun60i_serdes_configure,
	.verify		= sun60i_serdes_verify,
	.owner		= THIS_MODULE,
};

static int sun60i_serdes_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sun60i_serdes *serdes;
	struct phy_provider *provider;
	struct phy *phy;
	void __iomem *base;
	struct resource *res;
	int lane;

	serdes = devm_kzalloc(dev, sizeof(*serdes), GFP_KERNEL);
	if (!serdes)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	serdes->base = base;
	serdes->dev = dev;

	serdes->clk = devm_clk_get(dev, "ref_clk");
	if (IS_ERR(serdes->clk))
		return dev_err_probe(dev, PTR_ERR(serdes->clk),
				     "Failed to get ref_clk\n");

	serdes->bus_clk = devm_clk_get(dev, "bus_clk");
	if (IS_ERR(serdes->bus_clk))
		return dev_err_probe(dev, PTR_ERR(serdes->bus_clk),
				     "Failed to get bus_clk\n");

	serdes->supply = devm_regulator_get_optional(dev, "phy");
	if (IS_ERR(serdes->supply)) {
		if (PTR_ERR(serdes->supply) != -ENODEV)
			return dev_err_probe(dev, PTR_ERR(serdes->supply),
					     "Failed to get regulator\n");
		serdes->supply = NULL;
	}

	platform_set_drvdata(pdev, serdes);

	/* Register two PHYs, one per lane */
	for (lane = 0; lane < SERDES_NUM_LANES; lane++) {
		phy = devm_phy_create(dev, NULL, &sun60i_serdes_ops);
		if (IS_ERR(phy))
			return dev_err_probe(dev, PTR_ERR(phy),
					     "Failed to create PHY lane %d\n",
					     lane);

		phy_set_bus_width(phy, 0);
		phy_set_drvdata(phy, serdes);
		phy->id = lane;
		phy->init_args = 0;

		serdes->lane_in_use[lane] = false;
	}

	provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(provider))
		return dev_err_probe(dev, PTR_ERR(provider),
				     "Failed to register PHY provider\n");

	return 0;
}

static int sun60i_serdes_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id sun60i_serdes_of_match[] = {
	{ .compatible = "allwinner,sun60i-serdes" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, sun60i_serdes_of_match);

static struct platform_driver sun60i_serdes_driver = {
	.probe	= sun60i_serdes_probe,
	.remove	= sun60i_serdes_remove,
	.driver	= {
		.name		= "sun60i-serdes",
		.of_match_table	= sun60i_serdes_of_match,
	},
};
module_platform_driver(sun60i_serdes_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allwinner");
MODULE_DESCRIPTION("Allwinner sun60i SerDes PHY driver");
