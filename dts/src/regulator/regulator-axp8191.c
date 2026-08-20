// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * AXP8191 Regulator driver
 *
 * Supports all 16 regulator rails on the AXP8191 PMIC:
 *   4x DCDC (A/C/D/E) buck converters
 *   1x SW (5V boost)
 *   3x ALDO (1-3)
 *   4x BLDO (1-4)
 *   4x CLDO (1-4)
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>

/* Regulator enable and voltage register maps */
struct axp8191_reg_info {
	const char *name;
	u8 enable_reg;
	u8 enable_bit;
	u8 vol_reg;
	u8 vol_min;
	u8 vol_max;
	u8 vol_step;
	unsigned int min_uv;
	unsigned int max_uv;
};

#define AXPENT(_name, _ereg, _ebit, _vreg, _vmin, _vmax, _vstep, \
	       _minuv, _maxuv) \
	.name = _name, .enable_reg = _ereg, .enable_bit = _ebit, \
	.vol_reg = _vreg, .vol_min = _vmin, .vol_max = _vmax, \
	.vol_step = _vstep, .min_uv = _minuv, .max_uv = _maxuv

static const struct axp8191_reg_info axp8191_regs[] = {
	/* DCDC A: 0.6V-1.5V, 10mV step */
	[0] = { AXPENT("DCDC_A", 0x10, 7, 0x11, 0x00, 0x3f, 0x01,
			600000, 1520000), },
	/* DCDC C: 0.8V-1.84V, 10mV step */
	[1] = { AXPENT("DCDC_C", 0x12, 7, 0x13, 0x00, 0x3f, 0x01,
			800000, 1840000), },
	/* DCDC D: 0.5V-1.34V, 10mV step */
	[2] = { AXPENT("DCDC_D", 0x14, 7, 0x15, 0x00, 0x3f, 0x01,
			500000, 1340000), },
	/* DCDC E: 1.2V-2.0V, 10mV step */
	[3] = { AXPENT("DCDC_E", 0x16, 7, 0x17, 0x00, 0x3f, 0x01,
			1200000, 2000000), },
	/* ALDO1: 0.7V-3.3V, 100mV step */
	[4] = { AXPENT("ALDO1", 0x18, 7, 0x19, 0x00, 0x1c, 0x01,
			700000, 3300000), },
	/* ALDO2: 0.7V-3.3V, 100mV step */
	[5] = { AXPENT("ALDO2", 0x1a, 7, 0x1b, 0x00, 0x1c, 0x01,
			700000, 3300000), },
	/* ALDO3: 0.7V-3.3V, 100mV step */
	[6] = { AXPENT("ALDO3", 0x1c, 7, 0x1d, 0x00, 0x1c, 0x01,
			700000, 3300000), },
	/* BLDO1: 0.7V-3.3V, 100mV step */
	[7] = { AXPENT("BLDO1", 0x20, 7, 0x21, 0x00, 0x1c, 0x01,
			700000, 3300000), },
	/* BLDO2: 0.7V-3.3V, 100mV step */
	[8] = { AXPENT("BLDO2", 0x22, 7, 0x23, 0x00, 0x1c, 0x01,
			700000, 3300000), },
	/* BLDO3: 0.7V-3.3V, 100mV step */
	[9] = { AXPENT("BLDO3", 0x24, 7, 0x25, 0x00, 0x1c, 0x01,
			700000, 3300000), },
	/* BLDO4: 0.7V-3.3V, 100mV step */
	[10] = { AXPENT("BLDO4", 0x26, 7, 0x27, 0x00, 0x1c, 0x01,
			 700000, 3300000), },
	/* CLDO1: 0.7V-3.3V, 100mV step */
	[11] = { AXPENT("CLDO1", 0x28, 7, 0x29, 0x00, 0x1c, 0x01,
			 700000, 3300000), },
	/* CLDO2: 0.7V-3.3V, 100mV step */
	[12] = { AXPENT("CLDO2", 0x2a, 7, 0x2b, 0x00, 0x1c, 0x01,
			 700000, 3300000), },
	/* CLDO3: 0.7V-3.3V, 100mV step */
	[13] = { AXPENT("CLDO3", 0x2c, 7, 0x2d, 0x00, 0x1c, 0x01,
			 700000, 3300000), },
	/* CLDO4: 0.7V-3.3V, 100mV step */
	[14] = { AXPENT("CLDO4", 0x2e, 7, 0x2f, 0x00, 0x1c, 0x01,
			 700000, 3300000), },
	/* SW: 5V fixed boost */
	[15] = { AXPENT("SW", 0x30, 7, 0x31, 0x00, 0x00, 0x00,
			 5000000, 5000000), },
};

static unsigned int axp8191_map_voltage(struct regulator_dev *rdev,
					int min_uv, int max_uv)
{
	const struct axp8191_reg_info *info = rdev_get_drvdata(rdev);
	unsigned int voltage_range;
	unsigned int n_steps;
	unsigned int step_uv;
	unsigned int uv;
	int reg;

	step_uv = (info->max_uv - info->min_uv) /
		  (info->vol_max - info->vol_min + 1);

	if (min_uv < info->min_uv)
		min_uv = info->min_uv;
	if (max_uv > info->max_uv)
		max_uv = info->max_uv;

	n_steps = DIV_ROUND_UP(min_uv - info->min_uv, step_uv);
	uv = info->min_uv + n_steps * step_uv;

	if (uv > max_uv)
		return -EINVAL;

	reg = info->vol_min + n_steps;
	if (reg > info->vol_max)
		return -EINVAL;

	return uv;
}

static int axp8191_set_voltage(struct regulator_dev *rdev,
			       int min_uv, int max_uv, unsigned int *sel)
{
	const struct axp8191_reg_info *info = rdev_get_drvdata(rdev);
	unsigned int uv;
	int reg;

	if (info->vol_reg == 0)
		return -EINVAL;

	uv = axp8191_map_voltage(rdev, min_uv, max_uv);
	if (uv < 0)
		return uv;

	reg = (uv - info->min_uv) /
	      ((info->max_uv - info->min_uv) / (info->vol_max - info->vol_min + 1));
	reg += info->vol_min;

	return regmap_update_bits(rdev->regmap, info->vol_reg,
				 info->vol_max, reg);
}

static int axp8191_get_voltage(struct regulator_dev *rdev)
{
	const struct axp8191_reg_info *info = rdev_get_drvdata(rdev);
	unsigned int reg;
	unsigned int uv;
	unsigned int step_uv;
	int ret;

	if (info->vol_reg == 0)
		return info->min_uv;

	ret = regmap_read(rdev->regmap, info->vol_reg, &reg);
	if (ret)
		return ret;

	reg &= info->vol_max;

	step_uv = (info->max_uv - info->min_uv) /
		  (info->vol_max - info->vol_min + 1);

	uv = info->min_uv + (reg - info->vol_min) * step_uv;

	if (uv > info->max_uv)
		return info->max_uv;

	return uv;
}

static int axp8191_enable(struct regulator_dev *rdev)
{
	const struct axp8191_reg_info *info = rdev_get_drvdata(rdev);

	return regmap_update_bits(rdev->regmap, info->enable_reg,
				 BIT(info->enable_bit), BIT(info->enable_bit));
}

static int axp8191_disable(struct regulator_dev *rdev)
{
	const struct axp8191_reg_info *info = rdev_get_drvdata(rdev);

	return regmap_update_bits(rdev->regmap, info->enable_reg,
				 BIT(info->enable_bit), 0);
}

static int axp8191_is_enabled(struct regulator_dev *rdev)
{
	const struct axp8191_reg_info *info = rdev_get_drvdata(rdev);
	unsigned int val;
	int ret;

	ret = regmap_read(rdev->regmap, info->enable_reg, &val);
	if (ret)
		return ret;

	return !!(val & BIT(info->enable_bit));
}

static const struct regulator_ops axp8191_regulator_ops = {
	.enable		= axp8191_enable,
	.disable	= axp8191_disable,
	.is_enabled	= axp8191_is_enabled,
	.set_voltage	= axp8191_set_voltage,
	.get_voltage	= axp8191_get_voltage,
	.list_voltage	= regulator_list_voltage_linear_range,
};

#define AXP8191_RDESC(_name) \
	&((struct regulator_desc) { \
		.name = _name, \
		.ops = &axp8191_regulator_ops, \
		.type = REGULATOR_VOLTAGE, \
		.owner = THIS_MODULE, \
	})

static int axp8191_regulator_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regmap *regmap;
	struct regulator_dev *rdev;
	int i, ret;

	regmap = dev_get_regmap(dev->parent, NULL);
	if (!regmap)
		return -ENODEV;

	for (i = 0; i < ARRAY_SIZE(axp8191_regs); i++) {
		const struct axp8191_reg_info *info = &axp8191_regs[i];
		struct regulator_desc *desc;
		struct regulator_config config = {};

		desc = devm_kzalloc(dev, sizeof(*desc), GFP_KERNEL);
		if (!desc)
			return -ENOMEM;

		desc->name = info->name;
		desc->ops = &axp8191_regulator_ops;
		desc->type = REGULATOR_VOLTAGE;
		desc->owner = THIS_MODULE;
		desc->of_match = devm_kasprintf(dev, GFP_KERNEL,
						"axp8191-%s-ldo",
						info->name);
		if (!desc->of_match)
			return -ENOMEM;
		{
			char *p = (char *)desc->of_match;
			while (*p) {
				*p = tolower(*p);
				p++;
			}
		}

		config.dev = dev;
		config.regmap = regmap;

		rdev = devm_regulator_register(dev, desc, &config);
		if (IS_ERR(rdev)) {
			ret = dev_err_probe(dev, PTR_ERR(rdev),
					    "Failed to register %s\n",
					    info->name);
			return ret;
		}
	}

	return 0;
}

static const struct of_device_id axp8191_regulator_of_match[] = {
	{ .compatible = "allwinner,axp8191-regulator" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, axp8191_regulator_of_match);

static struct platform_driver axp8191_regulator_driver = {
	.probe	= axp8191_regulator_probe,
	.driver	= {
		.name		= "axp8191-regulator",
		.of_match_table	= axp8191_regulator_of_match,
	},
};
module_platform_driver(axp8191_regulator_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allwinner");
MODULE_DESCRIPTION("AXP8191 regulator driver");
