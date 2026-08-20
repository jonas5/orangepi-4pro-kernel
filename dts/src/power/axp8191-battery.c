// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * AXP8191 Battery driver
 *
 * Reports battery voltage, current, SOC, health, temperature, and
 * charge status via the Linux power_supply framework.
 *
 * The AXP8191 has ADC channels for battery voltage/current and
 * temperature monitoring, plus charger control/status registers.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

/* Register definitions (from MFD) */
#define AXP8191_CHARGER_CTRL1	0x40
#define AXP8191_CHARGER_CTRL2	0x41
#define AXP8191_CHARGER_CTRL3	0x42
#define AXP8191_CHARGER_STATUS	0x43
#define AXP8191_BATTERY_STATUS	0x44
#define AXP8191_CHARGE_CTL	0x45
#define AXP8191_ADC_EN1		0x50
#define AXP8191_ADC_EN2		0x51
#define AXP8191_ADC_EN3		0x52
#define AXP8191_VBUS_VOLT_H	0x56
#define AXP8191_VBUS_VOLT_L	0x57
#define AXP8191_BATT_VOLT_H	0x5a
#define AXP8191_BATT_VOLT_L	0x5b
#define AXP8191_BATT_CUR_H	0x5c
#define AXP8191_BATT_CUR_L	0x5d
#define AXP8191_TEMP_H		0x62
#define AXP8191_TEMP_L		0x63
#define AXP8191_INT_ENABLE1	0x70
#define AXP8191_INT_STATUS1	0x74

/* ADC enable bits */
#define AXP8191_ADC_EN_BATT_VOLT	BIT(7)
#define AXP8191_ADC_EN_BATT_CUR		BIT(6)
#define AXP8191_ADC_EN_TEMP		BIT(5)

/* Charger status bits */
#define AXP8191_CS_CHARGE_DONE		BIT(7)
#define AXP8191_CS_CHARGING		BIT(6)
#define AXP8191_CS_BATTERY_PRESENT	BIT(5)

/* Battery status bits */
#define AXP8191_BS_BATT_OV		BIT(7)
#define AXP8191_BS_BATT_UN		BIT(6)

/* Charger control bits */
#define AXP8191_CC1_CHARGE_ENABLE	BIT(7)
#define AXP8191_CC1_INPUT_CUR_MASK	GENMASK(6, 4)

/* Interrupt bits */
#define AXP8191_IRQ_VBUS_IN		BIT(2)
#define AXP8191_IRQ_VBUS_OUT		BIT(3)
#define AXP8191_IRQ_BATT_LOW		BIT(4)

struct axp8191_bat {
	struct device *dev;
	struct regmap *regmap;
	struct power_supply *bat;
	struct delayed_work work;
	unsigned int poll_interval;
};

static int axp8191_bat_get_vbat_voltage(struct axp8191_bat *bat)
{
	unsigned int h, l;
	int ret;

	ret = regmap_read(bat->regmap, AXP8191_BATT_VOLT_H, &h);
	if (ret)
		return ret;

	ret = regmap_read(bat->regmap, AXP8191_BATT_VOLT_L, &l);
	if (ret)
		return ret;

	/* 13-bit value, 1.1mV per step */
	return ((h & 0x1f) << 8 | l) * 1100;
}

static int axp8191_bat_get_current(struct axp8191_bat *bat)
{
	unsigned int h, l;
	int ret;

	ret = regmap_read(bat->regmap, AXP8191_BATT_CUR_H, &h);
	if (ret)
		return ret;

	ret = regmap_read(bat->regmap, AXP8191_BATT_CUR_L, &l);
	if (ret)
		return ret;

	/* 13-bit signed value, 0.5mA per step */
	int raw = (h & 0x1f) << 8 | l;

	/* Sign extend from 13-bit */
	if (raw & 0x1000)
		raw -= 0x2000;

	return raw * 500; /* microamps */
}

static int axp8191_bat_get_temp(struct axp8191_bat *bat)
{
	unsigned int h, l;
	int raw, ret;

	ret = regmap_read(bat->regmap, AXP8191_TEMP_H, &h);
	if (ret)
		return ret;

	ret = regmap_read(bat->regmap, AXP8191_TEMP_L, &l);
	if (ret)
		return ret;

	raw = ((h & 0x0f) << 8) | l;

	/* Convert to 0.1°C: common X-Powers formula is raw * 0.1 - 273.15 */
	return (raw - 264) * 10; /* Approximate: offset 264 = 0°C */
}

static int axp8191_bat_is_present(struct axp8191_bat *bat)
{
	unsigned int val;
	int ret;

	ret = regmap_read(bat->regmap, AXP8191_CHARGER_STATUS, &val);
	if (ret)
		return 1; /* Assume present if we can't read */

	return !(val & AXP8191_CS_BATTERY_PRESENT) ? 0 : 1;
}

static int axp8191_bat_get_charge_status(struct axp8191_bat *bat)
{
	unsigned int val;
	int ret;

	ret = regmap_read(bat->regmap, AXP8191_CHARGER_STATUS, &val);
	if (ret)
		return POWER_SUPPLY_STATUS_DISCHARGING;

	if (val & AXP8191_CS_CHARGE_DONE)
		return POWER_SUPPLY_STATUS_FULL;
	if (val & AXP8191_CS_CHARGING)
		return POWER_SUPPLY_STATUS_CHARGING;

	return POWER_SUPPLY_STATUS_DISCHARGING;
}

/*
 * Simple SOC estimation from voltage using a 2-point linear approximation
 * for a single-cell Li-ion battery (3.0V empty, 4.2V full).
 * For production use, this should be replaced with a proper fuel gauge
 * algorithm or the AXP515's SOC voltage curve from the DTS.
 */
static int axp8191_bat_get_soc(struct axp8191_bat *bat, int vbat_uv)
{
	int soc;

	/* Clamp to Li-ion voltage range */
	if (vbat_uv < 3000000)
		return 0;
	if (vbat_uv >= 4200000)
		return 100;

	/* Linear interpolation: 3.0V = 0%, 4.2V = 100% */
	soc = (vbat_uv - 3000000) * 100 / (4200000 - 3000000);

	/* Clamp */
	if (soc < 0)
		soc = 0;
	if (soc > 100)
		soc = 100;

	return soc;
}

static int axp8191_bat_get_capacity_level(struct axp8191_bat *bat, int soc)
{
	if (soc <= 5)
		return POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
	if (soc <= 15)
		return POWER_SUPPLY_CAPACITY_LEVEL_LOW;
	if (soc >= 95)
		return POWER_SUPPLY_CAPACITY_LEVEL_FULL;
	return POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
}

static int axp8191_bat_get_health(struct axp8191_bat *bat)
{
	int temp = axp8191_bat_get_temp(bat);

	/* Temperature in 0.1°C */
	if (temp > 600)  /* > 60°C */
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	if (temp < 0)   /* < 0°C */
		return POWER_SUPPLY_HEALTH_COLD;
	if (temp > 450) /* > 45°C */
		return POWER_SUPPLY_HEALTH_WARM;

	return POWER_SUPPLY_HEALTH_GOOD;
}

enum bat_properties {
	PROP_STATUS,
	PROP_PRESENT,
	PROP_VOLTAGE_NOW,
	PROP_CURRENT_NOW,
	PROP_CAPACITY,
	PROP_CAPACITY_LEVEL,
	PROP_TEMP,
	PROP_TECHNOLOGY,
	PROP_HEALTH,
	PROP_CHARGE_CONTROL_LIMIT,
	PROP_CHARGE_CONTROL_LIMIT_MAX,
};

static enum power_supply_property axp8191_bat_props[] = {
	[PROP_STATUS] = POWER_SUPPLY_PROP_STATUS,
	[PROP_PRESENT] = POWER_SUPPLY_PROP_PRESENT,
	[PROP_VOLTAGE_NOW] = POWER_SUPPLY_PROP_VOLTAGE_NOW,
	[PROP_CURRENT_NOW] = POWER_SUPPLY_PROP_CURRENT_NOW,
	[PROP_CAPACITY] = POWER_SUPPLY_PROP_CAPACITY,
	[PROP_CAPACITY_LEVEL] = POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	[PROP_TEMP] = POWER_SUPPLY_PROP_TEMP,
	[PROP_TECHNOLOGY] = POWER_SUPPLY_PROP_TECHNOLOGY,
	[PROP_HEALTH] = POWER_SUPPLY_PROP_HEALTH,
	[PROP_CHARGE_CONTROL_LIMIT] = POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
	[PROP_CHARGE_CONTROL_LIMIT_MAX] = POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX,
};

static int axp8191_bat_get_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    union power_supply_propval *val)
{
	struct axp8191_bat *bat = power_supply_get_drvdata(psy);
	int vbat, ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = axp8191_bat_get_charge_status(bat);
		return 0;

	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = axp8191_bat_is_present(bat);
		return 0;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = axp8191_bat_get_vbat_voltage(bat);
		return 0;

	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = axp8191_bat_get_current(bat);
		return 0;

	case POWER_SUPPLY_PROP_CAPACITY:
		vbat = axp8191_bat_get_vbat_voltage(bat);
		val->intval = axp8191_bat_get_soc(bat, vbat);
		return 0;

	case POWER_SUPPLY_PROP_CAPACITY_LEVEL:
		vbat = axp8191_bat_get_vbat_voltage(bat);
		val->intval = axp8191_bat_get_capacity_level(
			bat, axp8191_bat_get_soc(bat, vbat));
		return 0;

	case POWER_SUPPLY_PROP_TEMP:
		val->intval = axp8191_bat_get_temp(bat);
		return 0;

	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LIION;
		return 0;

	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = axp8191_bat_get_health(bat);
		return 0;

	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		/* Read charger enable status */
		ret = regmap_read(bat->regmap, AXP8191_CHARGER_CTRL1, &val->intval);
		if (ret)
			return ret;
		val->intval = !!(val->intval & AXP8191_CC1_CHARGE_ENABLE);
		return 0;

	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		val->intval = 1; /* 0=disable, 1=enable */
		return 0;

	default:
		return -EINVAL;
	}
}

static int axp8191_bat_set_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    const union power_supply_propval *val)
{
	struct axp8191_bat *bat = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		regmap_update_bits(bat->regmap, AXP8191_CHARGER_CTRL1,
				   AXP8191_CC1_CHARGE_ENABLE,
				   val->intval ? AXP8191_CC1_CHARGE_ENABLE : 0);
		return 0;
	default:
		return -EINVAL;
	}
}

static int axp8191_bat_property_is_writeable(struct power_supply *psy,
					     enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		return 1;
	default:
		return 0;
	}
}

static void axp8191_bat_work_func(struct work_struct *work)
{
	struct axp8191_bat *bat = container_of(work, struct axp8191_bat,
					      work.work);

	power_supply_changed(bat->bat);

	schedule_delayed_work(&bat->bat->work,
			      msecs_to_jiffies(bat->poll_interval));
}

static const struct power_supply_desc axp8191_bat_desc = {
	.name = "axp8191-battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = axp8191_bat_props,
	.num_properties = ARRAY_SIZE(axp8191_bat_props),
	.get_property = axp8191_bat_get_property,
	.set_property = axp8191_bat_set_property,
	.property_is_writeable = axp8191_bat_property_is_writeable,
	.no_thermal = true,
};

static int axp8191_battery_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct axp8191_bat *bat;
	struct power_supply_config psy_cfg = {};
	unsigned int val;

	bat = devm_kzalloc(dev, sizeof(*bat), GFP_KERNEL);
	if (!bat)
		return -ENOMEM;

	bat->dev = dev;
	bat->regmap = dev_get_regmap(dev->parent, NULL);
	if (!bat->regmap)
		return -ENODEV;

	bat->poll_interval = 30000; /* 30 seconds */

	platform_set_drvdata(pdev, bat);

	/* Enable battery voltage, current, and temperature ADC channels */
	regmap_update_bits(bat->regmap, AXP8191_ADC_EN2,
			   AXP8191_ADC_EN_BATT_VOLT |
			   AXP8191_ADC_EN_BATT_CUR |
			   AXP8191_ADC_EN_TEMP,
			   AXP8191_ADC_EN_BATT_VOLT |
			   AXP8191_ADC_EN_BATT_CUR |
			   AXP8191_ADC_EN_TEMP);

	/* Read initial charger status */
	regmap_read(bat->regmap, AXP8191_CHARGER_STATUS, &val);
	dev_info(dev, "AXP8191 battery: charger_status=0x%02x\n", val);

	/* Read chip version for debug */
	regmap_read(bat->regmap, AXP8191_CHARGER_CTRL1, &val);
	dev_dbg(dev, "AXP8191 battery: charger_ctrl1=0x%02x\n", val);

	psy_cfg.drv_data = bat;
	psy_cfg.fwnode = dev_fwnode(dev);

	bat->bat = devm_power_supply_register(dev, &axp8191_bat_desc,
					      &psy_cfg);
	if (IS_ERR(bat->bat))
		return dev_err_probe(dev, PTR_ERR(bat->bat),
				     "Failed to register battery supply\n");

	INIT_DELAYED_WORK(&bat->bat->work, axp8191_bat_work_func);

	dev_dbg(dev, "AXP8191 battery driver registered (poll=%ums)\n",
		bat->poll_interval);
	return 0;
}

static const struct of_device_id axp8191_battery_of_match[] = {
	{ .compatible = "allwinner,axp8191-battery" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, axp8191_battery_of_match);

static struct platform_driver axp8191_battery_driver = {
	.driver = {
		.name = "axp8191-battery",
		.of_match_table = axp8191_battery_of_match,
	},
	.probe = axp8191_battery_probe,
};
module_platform_driver(axp8191_battery_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allwinner");
MODULE_DESCRIPTION("AXP8191 Battery driver");
