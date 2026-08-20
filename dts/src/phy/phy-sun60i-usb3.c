// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Allwinner sun60i USB3 PHY driver
 *
 * Configures SerDes PHY lane for USB3 (SuperSpeed) operation on A733.
 */

#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

/* USB3 PHY specific register offsets (mapped via SerDes) */
#define USB3_PHY_CTRL		0x0100
#define USB3_PHY_STATUS		0x0104
#define USB3_PHY_TRIM		0x0108
#define USB3_PHY_TUNE		0x010c

/* Control register bits */
#define USB3_CTRL_MODE_HOST	0x00
#define USB3_CTRL_MODE_DEV	0x01
#define USB3_CTRL_SSC_EN	BIT(4)
#define USB3_CTRL_RX_POL		BIT(5)
#define USB3_CTRL_TX_POL		BIT(6)
#define USB3_CTRL_SS_EN		BIT(7)
#define USB3_CTRL_POWER_ON	BIT(8)

/* Status register bits */
#define USB3_STATUS_PLL_LOCK	BIT(0)
#define USB3_STATUS_RX_READY	BIT(1)
#define USB3_STATUS_TX_READY	BIT(2)

#define USB3_PHY_TIMEOUT_US	10000

struct sun60i_usb3_phy {
	struct device		*dev;
	struct phy		*serdes;
	struct clk		*clk;
	struct regulator	*supply;
	enum phy_mode		mode;
	void __iomem		*base;
};

static int sun60i_usb3_phy_power_on(struct phy *phy)
{
	struct sun60i_usb3_phy *priv = phy_get_drvdata(phy);
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

	/* Configure SerDes for USB3 mode */
	if (priv->serdes) {
		ret = phy_set_mode_ext(priv->serdes, PHY_MODE_USB_HOST_SS, 0);
		if (ret) {
			dev_err(priv->dev, "Failed to set SerDes mode: %d\n",
				ret);
			goto err_disable_clk;
		}
	}

	/* Configure USB3 PHY */
	val = USB3_CTRL_POWER_ON | USB3_CTRL_SSC_EN | USB3_CTRL_SS_EN;

	if (priv->mode == PHY_MODE_USB_DEVICE)
		val |= USB3_CTRL_MODE_DEV;
	else
		val |= USB3_CTRL_MODE_HOST;

	writel(val, priv->base + USB3_PHY_CTRL);

	/* Wait for PLL lock */
	ret = readl_poll_timeout(priv->base + USB3_PHY_STATUS, val,
				 (val & USB3_STATUS_PLL_LOCK), 100,
				 USB3_PHY_TIMEOUT_US);
	if (ret) {
		dev_err(priv->dev, "USB3 PHY PLL not locked\n");
		goto err_disable_clk;
	}

	return 0;

err_disable_clk:
	clk_disable_unprepare(priv->clk);
	if (priv->supply)
		regulator_disable(priv->supply);
	return ret;
}

static int sun60i_usb3_phy_power_off(struct phy *phy)
{
	struct sun60i_usb3_phy *priv = phy_get_drvdata(phy);

	writel(0, priv->base + USB3_PHY_CTRL);

	clk_disable_unprepare(priv->clk);

	if (priv->supply)
		regulator_disable(priv->supply);

	return 0;
}

static int sun60i_usb3_phy_set_mode(struct phy *phy, enum phy_mode mode,
				    int submode)
{
	struct sun60i_usb3_phy *priv = phy_get_drvdata(phy);

	switch (mode) {
	case PHY_MODE_USB_HOST_SS:
	case PHY_MODE_USB_HOST_HS:
	case PHY_MODE_USB_HOST_FS:
		priv->mode = mode;
		break;
	case PHY_MODE_USB_DEVICE:
		priv->mode = mode;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct phy_ops sun60i_usb3_phy_ops = {
	.power_on	= sun60i_usb3_phy_power_on,
	.power_off	= sun60i_usb3_phy_power_off,
	.set_mode	= sun60i_usb3_phy_set_mode,
	.owner		= THIS_MODULE,
};

static int sun60i_usb3_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sun60i_usb3_phy *priv;
	struct phy_provider *provider;
	struct phy *phy;
	struct resource *res;

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

	priv->supply = devm_regulator_get_optional(dev, "vbus");
	if (IS_ERR(priv->supply)) {
		if (PTR_ERR(priv->supply) != -ENODEV)
			return dev_err_probe(dev, PTR_ERR(priv->supply),
					     "Failed to get vbus supply\n");
		priv->supply = NULL;
	}

	priv->serdes = devm_phy_optional_get(dev, "serdes");
	if (IS_ERR(priv->serdes))
		return dev_err_probe(dev, PTR_ERR(priv->serdes),
				     "Failed to get SerDes PHY\n");

	priv->mode = PHY_MODE_USB_HOST_SS;

	platform_set_drvdata(pdev, priv);

	phy = devm_phy_create(dev, NULL, &sun60i_usb3_phy_ops);
	if (IS_ERR(phy))
		return dev_err_probe(dev, PTR_ERR(phy),
				     "Failed to create USB3 PHY\n");

	phy_set_drvdata(phy, priv);

	provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(provider))
		return dev_err_probe(dev, PTR_ERR(provider),
				     "Failed to register PHY provider\n");

	return 0;
}

static const struct of_device_id sun60i_usb3_phy_of_match[] = {
	{ .compatible = "allwinner,sun60i-a733-usb3-phy" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, sun60i_usb3_phy_of_match);

static struct platform_driver sun60i_usb3_phy_driver = {
	.probe	= sun60i_usb3_phy_probe,
	.driver	= {
		.name		= "sun60i-usb3-phy",
		.of_match_table	= sun60i_usb3_phy_of_match,
	},
};
module_platform_driver(sun60i_usb3_phy_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allwinner");
MODULE_DESCRIPTION("Allwinner sun60i USB3 PHY driver for A733");
