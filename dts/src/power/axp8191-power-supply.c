// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * AXP8191 Power Supply driver
 *
 * Reports VBUS/AC adapter online status, voltage, and current.
 * The AXP8191 has ADC channels for VBUS voltage/current monitoring
 * and status registers for power source detection.
 *
 * Supports charger_mode=kernel parameter:
 *   powerbank - limit input current to 500mA (safe for USB power banks)
 *   fast      - allow up to 1500mA input current (wall adapter)
 *   (empty)  - default 1000mA
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>

/* Register definitions (from MFD) */
#define AXP8191_POWER_STATUS	0x02
#define AXP8191_VBUS_STATUS	0x03
#define AXP8191_CHARGER_CTRL1	0x40
#define AXP8191_ADC_EN1		0x50
#define AXP8191_VBUS_VOLT_H	0x56
#define AXP8191_VBUS_VOLT_L	0x57
#define AXP8191_VBUS_CUR_H	0x58
#define AXP8191_VBUS_CUR_L	0x59
#define AXP8191_ADC_SPEED	0x53

/* ADC enable bits */
#define AXP8191_ADC_EN_VBUS_VOLT	BIT(7)
#define AXP8191_ADC_EN_VBUS_CUR		BIT(6)

/* POWER_STATUS bits */
#define AXP8191_PS_VBUS_PRESENT		BIT(7)
#define AXP8191_PS_ACIN_PRESENT		BIT(6)

/* Charger CTRL1 bits */
#define AXP8191_CC1_CHARGE_ENABLE	BIT(7)
#define AXP8191_CC1_IINLIM_MASK	GENMASK(6, 4)
#define AXP8191_CC1_IINLIM_100MA	0
#define AXP8191_CC1_IINLIM_500MA	BIT(4)
#define AXP8191_CC1_IINLIM_1000MA	BIT(5)
#define AXP8191_CC1_IINLIM_1500MA	(BIT(5) | BIT(4))

static char *charger_mode;
module_param(charger_mode, charp, 0444);
MODULE_PARM_DESC(charger_mode, "Charger mode: powerbank (500mA), fast (1500mA)");

struct axp8191_psy {
	struct device *dev;
	struct regmap *regmap;
	struct power_supply *ac;
	struct power_supply *usb;
};

static int axp8191_psy_get_vbus_voltage(struct axp8191_psy *psy)
{
	unsigned int h, l;
	int ret;

	ret = regmap_read(psy->regmap, AXP8191_VBUS_VOLT_H, &h);
	if (ret)
		return ret;

	ret = regmap_read(psy->regmap, AXP8191_VBUS_VOLT_L, &l);
	if (ret)
		return ret;

	/* 13-bit value, 1.7mV per step */
	return ((h & 0x1f) << 8 | l) * 1700;
}

static int axp8191_psy_get_vbus_current(struct axp8191_psy *psy)
{
	unsigned int h, l;
	int ret;

	ret = regmap_read(psy->regmap, AXP8191_VBUS_CUR_H, &h);
	if (ret)
		return ret;

	ret = regmap_read(psy->regmap, AXP8191_VBUS_CUR_L, &l);
	if (ret)
		return ret;

	/* 13-bit value, 0.375mA per step */
	return ((h & 0x1f) << 8 | l) * 375;
}

static int axp8191_psy_is_online(struct axp8191_psy *psy)
{
	unsigned int val;
	int ret;

	ret = regmap_read(psy->regmap, AXP8191_POWER_STATUS, &val);
	if (ret)
		return 0;

	return !!(val & AXP8191_PS_VBUS_PRESENT);
}

static enum power_supply_property axp8191_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
};

static int axp8191_usb_get_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
	struct axp8191_psy *psy_data = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = axp8191_psy_is_online(psy_data);
		return 0;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = axp8191_psy_get_vbus_voltage(psy_data);
		return 0;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = axp8191_psy_get_vbus_current(psy_data);
		return 0;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = 4200000; /* USB standard */
		return 0;
	default:
		return -EINVAL;
	}
}

static enum power_supply_property axp8191_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

static int axp8191_ac_get_property(struct power_supply *psy,
				   enum power_supply_property psp,
				   union power_supply_propval *val)
{
	struct axp8191_psy *psy_data = power_supply_get_drvdata(psy);
	unsigned int reg;
	int ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = regmap_read(psy_data->regmap, AXP8191_POWER_STATUS, &reg);
		if (ret)
			return ret;
		val->intval = !!(reg & AXP8191_PS_ACIN_PRESENT);
		return 0;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = axp8191_psy_get_vbus_voltage(psy_data);
		return 0;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = axp8191_psy_get_vbus_current(psy_data);
		return 0;
	default:
		return -EINVAL;
	}
}

static const struct power_supply_desc axp8191_usb_desc = {
	.name = "axp8191-usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.properties = axp8191_usb_props,
	.num_properties = ARRAY_SIZE(axp8191_usb_props),
	.get_property = axp8191_usb_get_property,
};

static const struct power_supply_desc axp8191_ac_desc = {
	.name = "axp8191-ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties = axp8191_ac_props,
	.num_properties = ARRAY_SIZE(axp8191_ac_props),
	.get_property = axp8191_ac_get_property,
};

static void axp8191_psy_set_charger_current(struct axp8191_psy *psy)
{
	u32 iinlim;

	if (!charger_mode)
		return;

	if (strcmp(charger_mode, "powerbank") == 0) {
		iinlim = AXP8191_CC1_IINLIM_500MA;
		dev_info(psy->dev, "charger_mode=powerbank: limiting input to 500mA\n");
	} else if (strcmp(charger_mode, "fast") == 0) {
		iinlim = AXP8191_CC1_IINLIM_1500MA;
		dev_info(psy->dev, "charger_mode=fast: allowing input up to 1500mA\n");
	} else {
		dev_warn(psy->dev, "unknown charger_mode='%s', using default\n",
			 charger_mode);
		return;
	}

	regmap_update_bits(psy->regmap, AXP8191_CHARGER_CTRL1,
			   AXP8191_CC1_IINLIM_MASK, iinlim);
}

static int axp8191_power_supply_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct axp8191_psy *psy_data;
	struct power_supply_config usb_cfg = {}, ac_cfg = {};

	psy_data = devm_kzalloc(dev, sizeof(*psy_data), GFP_KERNEL);
	if (!psy_data)
		return -ENOMEM;

	psy_data->dev = dev;
	psy_data->regmap = dev_get_regmap(dev->parent, NULL);
	if (!psy_data->regmap)
		return -ENODEV;

	platform_set_drvdata(pdev, psy_data);

	/* Enable VBUS ADC channels */
	regmap_update_bits(psy_data->regmap, AXP8191_ADC_EN1,
			   AXP8191_ADC_EN_VBUS_VOLT | AXP8191_ADC_EN_VBUS_CUR,
			   AXP8191_ADC_EN_VBUS_VOLT | AXP8191_ADC_EN_VBUS_CUR);

	/* Apply charger mode from kernel command line */
	axp8191_psy_set_charger_current(psy_data);

	usb_cfg.drv_data = psy_data;
	usb_cfg.fwnode = dev_fwnode(dev);

	psy_data->usb = devm_power_supply_register(dev, &axp8191_usb_desc,
						   &usb_cfg);
	if (IS_ERR(psy_data->usb))
		return dev_err_probe(dev, PTR_ERR(psy_data->usb),
				     "Failed to register USB supply\n");

	ac_cfg.drv_data = psy_data;
	ac_cfg.fwnode = dev_fwnode(dev);

	psy_data->ac = devm_power_supply_register(dev, &axp8191_ac_desc,
						  &ac_cfg);
	if (IS_ERR(psy_data->ac))
		return dev_err_probe(dev, PTR_ERR(psy_data->ac),
				     "Failed to register AC supply\n");

	dev_dbg(dev, "AXP8191 power supply driver registered\n");
	return 0;
}

static const struct of_device_id axp8191_power_supply_of_match[] = {
	{ .compatible = "allwinner,axp8191-power-supply" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, axp8191_power_supply_of_match);

static struct platform_driver axp8191_power_supply_driver = {
	.driver = {
		.name = "axp8191-power-supply",
		.of_match_table = axp8191_power_supply_of_match,
	},
	.probe = axp8191_power_supply_probe,
};
module_platform_driver(axp8191_power_supply_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allwinner");
MODULE_DESCRIPTION("AXP8191 Power Supply driver");
MODULE_PARM_DESC(charger_mode, "Charger mode: powerbank (500mA), fast (1500mA)");
