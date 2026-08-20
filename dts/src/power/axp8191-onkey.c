// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * AXP8191 Onkey driver
 *
 * Handles power button events on AXP8191 PMIC:
 *   - Short press: triggers KEY_POWER event (can be used for shutdown)
 *   - Long press: triggers KEY_SUSPEND event (can be used for suspend)
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/regmap.h>
#include <linux/delay.h>

/* Register definitions */
#define AXP8191_INT_ENABLE2	0x71
#define AXP8191_INT_STATUS2	0x75

/* Interrupt bits */
#define AXP8191_IRQ_BTN_SHORT	BIT(0)
#define AXP8191_IRQ_BTN_LONG	BIT(1)

struct axp8191_onkey {
	struct device *dev;
	struct regmap *regmap;
	struct input_dev *input;
	int irq;
};

static irqreturn_t axp8191_onkey_irq(int irq, void *data)
{
	struct axp8191_onkey *onkey = data;
	unsigned int val;
	int ret;

	ret = regmap_read(onkey->regmap, AXP8191_INT_STATUS2, &val);
	if (ret)
		return IRQ_NONE;

	if (val & AXP8191_IRQ_BTN_SHORT) {
		dev_dbg(onkey->dev, "Power button short press\n");
		input_report_key(onkey->input, KEY_POWER, 1);
		input_sync(onkey->input);
		input_report_key(onkey->input, KEY_POWER, 0);
		input_sync(onkey->input);
		regmap_write(onkey->regmap, AXP8191_INT_STATUS2,
			     AXP8191_IRQ_BTN_SHORT);
	}

	if (val & AXP8191_IRQ_BTN_LONG) {
		dev_dbg(onkey->dev, "Power button long press\n");
		input_report_key(onkey->input, KEY_SUSPEND, 1);
		input_sync(onkey->input);
		input_report_key(onkey->input, KEY_SUSPEND, 0);
		input_sync(onkey->input);
		regmap_write(onkey->regmap, AXP8191_INT_STATUS2,
			     AXP8191_IRQ_BTN_LONG);
	}

	return IRQ_HANDLED;
}

static int axp8191_onkey_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct axp8191_onkey *onkey;
	int ret;

	onkey = devm_kzalloc(dev, sizeof(*onkey), GFP_KERNEL);
	if (!onkey)
		return -ENOMEM;

	onkey->dev = dev;
	onkey->regmap = dev_get_regmap(dev->parent, NULL);
	if (!onkey->regmap)
		return -ENODEV;

	onkey->irq = platform_get_irq(pdev, 0);
	if (onkey->irq < 0)
		return dev_err_probe(dev, onkey->irq, "Failed to get IRQ\n");

	/* Setup input device */
	onkey->input = devm_input_allocate_device(dev);
	if (!onkey->input)
		return -ENOMEM;

	onkey->input->name = "axp8191-onkey";
	onkey->input->phys = "axp8191-onkey/input0";
	onkey->input->id.bustype = BUS_I2C;
	onkey->input->id.vendor = 0x0001;
	onkey->input->id.product = 0x0001;

	input_set_capability(onkey->input, EV_KEY, KEY_POWER);
	input_set_capability(onkey->input, EV_KEY, KEY_SUSPEND);

	ret = devm_request_threaded_irq(dev, onkey->irq, NULL,
					axp8191_onkey_irq,
					IRQF_TRIGGER_LOW | IRQF_ONESHOT,
					"axp8191-onkey", onkey);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to request IRQ\n");

	ret = input_register_device(onkey->input);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to register input\n");

	platform_set_drvdata(pdev, onkey);

	dev_dbg(dev, "AXP8191 onkey driver registered\n");
	return 0;
}

static const struct of_device_id axp8191_onkey_of_match[] = {
	{ .compatible = "allwinner,axp8191-onkey" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, axp8191_onkey_of_match);

static struct platform_driver axp8191_onkey_driver = {
	.driver = {
		.name = "axp8191-onkey",
		.of_match_table = axp8191_onkey_of_match,
	},
	.probe = axp8191_onkey_probe,
};
module_platform_driver(axp8191_onkey_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allwinner");
MODULE_DESCRIPTION("AXP8191 Onkey driver");
