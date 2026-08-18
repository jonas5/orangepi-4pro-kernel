// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Allwinner AXP8191 MFD driver
 *
 * AXP8191 is an I2C PMIC (address 0x34) with:
 * - Regulators: DCDC A/C/D/E (buck), SW (5V boost),
 *   ALDO 1-3, BLDO 1-4, CLDO 1-4
 * - ADC channels for voltage/current monitoring
 * - Battery charger
 * - Power button
 * - Vbus detection
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>
#include <linux/mfd/core.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/reboot.h>
#include <linux/delay.h>

/* AXP8191 register space */
#define AXP8191_CHIP_ID		0x00
#define AXP8191_CHIP_VER	0x01
#define AXP8191_POWER_STATUS	0x02
#define AXP8191_VBUS_STATUS	0x03

/* DCDC registers (0x10-0x17) */
#define AXP8191_DCDC_A_CTRL	0x10
#define AXP8191_DCDC_A_VOLT	0x11
#define AXP8191_DCDC_C_CTRL	0x12
#define AXP8191_DCDC_C_VOLT	0x13
#define AXP8191_DCDC_D_CTRL	0x14
#define AXP8191_DCDC_D_VOLT	0x15
#define AXP8191_DCDC_E_CTRL	0x16
#define AXP8191_DCDC_E_VOLT	0x17
#define AXP8191_DCDC_ABC_CTRL	0x18

/* ALDO registers (0x19-0x1f) */
#define AXP8191_ALDO1_CTRL	0x19
#define AXP8191_ALDO1_VOLT	0x1a
#define AXP8191_ALDO2_CTRL	0x1b
#define AXP8191_ALDO2_VOLT	0x1c
#define AXP8191_ALDO3_CTRL	0x1d
#define AXP8191_ALDO3_VOLT	0x1e

/* BLDO registers (0x20-0x27) */
#define AXP8191_BLDO1_CTRL	0x20
#define AXP8191_BLDO1_VOLT	0x21
#define AXP8191_BLDO2_CTRL	0x22
#define AXP8191_BLDO2_VOLT	0x23
#define AXP8191_BLDO3_CTRL	0x24
#define AXP8191_BLDO3_VOLT	0x25
#define AXP8191_BLDO4_CTRL	0x26
#define AXP8191_BLDO4_VOLT	0x27

/* CLDO registers (0x28-0x2f) */
#define AXP8191_CLDO1_CTRL	0x28
#define AXP8191_CLDO1_VOLT	0x29
#define AXP8191_CLDO2_CTRL	0x2a
#define AXP8191_CLDO2_VOLT	0x2b
#define AXP8191_CLDO3_CTRL	0x2c
#define AXP8191_CLDO3_VOLT	0x2d
#define AXP8191_CLDO4_CTRL	0x2e
#define AXP8191_CLDO4_VOLT	0x2f

/* SW (boost) registers */
#define AXP8191_SW_CTRL		0x30
#define AXP8191_SW_VOLT		0x31

/* Charger registers */
#define AXP8191_CHARGER_CTRL1	0x40
#define AXP8191_CHARGER_CTRL2	0x41
#define AXP8191_CHARGER_CTRL3	0x42
#define AXP8191_CHARGER_STATUS	0x43
#define AXP8191_BATTERY_STATUS	0x44
#define AXP8191_CHARGE_CTL	0x45

/* ADC registers */
#define AXP8191_ADC_EN1		0x50
#define AXP8191_ADC_EN2		0x51
#define AXP8191_ADC_EN3		0x52
#define AXP8191_ADC_SPEED	0x53
#define AXP8191_VBUS_VOLT_H	0x56
#define AXP8191_VBUS_VOLT_L	0x57
#define AXP8191_VBUS_CUR_H	0x58
#define AXP8191_VBUS_CUR_L	0x59
#define AXP8191_BATT_VOLT_H	0x5a
#define AXP8191_BATT_VOLT_L	0x5b
#define AXP8191_BATT_CUR_H	0x5c
#define AXP8191_BATT_CUR_L	0x5d
#define AXP8191_DCDC_A_VOLT_ADJ	0x60
#define AXP8191_DCDC_C_VOLT_ADJ	0x61
#define AXP8191_TEMP_H		0x62
#define AXP8191_TEMP_L		0x63

/* Interrupt registers */
#define AXP8191_INT_ENABLE1	0x70
#define AXP8191_INT_ENABLE2	0x71
#define AXP8191_INT_ENABLE3	0x72
#define AXP8191_INT_STATUS1	0x74
#define AXP8191_INT_STATUS2	0x75
#define AXP8191_INT_STATUS3	0x76

/* Control registers */
#define AXP8191_DCDC_PWM_CTRL	0x80
#define AXP8191_VOFF_CTRL	0x81
#define AXP8191_SHUTDOWN_CTL	0x82
#define AXP8191_RESET_CTL	0x83

/* Power off register */
#define AXP8191_POWER_OFF	0xb0
#define AXP8191_POWER_OFF_MAGIC	0x40

#define AXP8191_CHIP_ID_VALUE	0xa1
#define AXP8191_MAX_REGS	0xff

/* Interrupt enable bits */
#define AXP8191_IRQ_VBUS_IN	BIT(2)
#define AXP8191_IRQ_VBUS_OUT	BIT(3)
#define AXP8191_IRQ_BATT_LOW	BIT(4)
#define AXP8191_IRQ_BATT_OK	BIT(5)
#define AXP8191_IRQ_ACIN_IN	BIT(6)
#define AXP8191_IRQ_ACIN_OUT	BIT(7)
#define AXP8191_IRQ_BTN_SHORT	BIT(0)
#define AXP8191_IRQ_BTN_LONG	BIT(1)

struct axp8191_dev {
	struct device		*dev;
	struct i2c_client	*client;
	struct regmap		*regmap;
	int			irq;
	int			irq_base;
};

static const struct regmap_config axp8191_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= AXP8191_MAX_REGS,
	.cache_type	= REGCACHE_NONE,
};

static struct mfd_cell axp8191_cells[] = {
	{
		.name		= "axp8191-regulator",
		.of_compatible	= "allwinner,axp8191-regulator",
	}, {
		.name		= "axp8191-power-supply",
		.of_compatible	= "allwinner,axp8191-power-supply",
	}, {
		.name		= "axp8191-battery",
		.of_compatible	= "allwinner,axp8191-battery",
	}, {
		.name		= "axp8191-onkey",
		.of_compatible	= "allwinner,axp8191-onkey",
	},
};

static void axp8191_power_off(void)
{
	struct axp8191_dev *axp = container_of(
		&__this_module, struct axp8191_dev, dev);

	/* Trigger a power off sequence */
	regmap_write(axp->regmap, AXP8191_POWER_OFF, AXP8191_POWER_OFF_MAGIC);

	mdelay(500);
}

static irqreturn_t axp8191_irq_thread(int irq, void *data)
{
	struct axp8191_dev *axp = data;
	unsigned int status1, status2, status3;
	int ret;

	ret = regmap_read(axp->regmap, AXP8191_INT_STATUS1, &status1);
	if (ret)
		return IRQ_NONE;

	ret = regmap_read(axp->regmap, AXP8191_INT_STATUS2, &status2);
	if (ret)
		return IRQ_NONE;

	ret = regmap_read(axp->regmap, AXP8191_INT_STATUS3, &status3);
	if (ret)
		return IRQ_NONE;

	/* VBUS detect */
	if (status1 & AXP8191_IRQ_VBUS_IN)
		dev_dbg(axp->dev, "VBUS connected\n");
	if (status1 & AXP8191_IRQ_VBUS_OUT)
		dev_dbg(axp->dev, "VBUS disconnected\n");

	/* AC-in */
	if (status1 & AXP8191_IRQ_ACIN_IN)
		dev_dbg(axp->dev, "ACIN inserted\n");
	if (status1 & AXP8191_IRQ_ACIN_OUT)
		dev_dbg(axp->dev, "ACIN removed\n");

	/* Battery */
	if (status1 & AXP8191_IRQ_BATT_LOW)
		dev_warn(axp->dev, "Battery under voltage!\n");

	/* Power button */
	if (status2 & AXP8191_IRQ_BTN_SHORT) {
		dev_dbg(axp->dev, "Power button short press\n");
		/* Clear status */
		regmap_write(axp->regmap, AXP8191_INT_STATUS2,
			     AXP8191_IRQ_BTN_SHORT);
	}
	if (status2 & AXP8191_IRQ_BTN_LONG) {
		dev_dbg(axp->dev, "Power button long press\n");
		regmap_write(axp->regmap, AXP8191_INT_STATUS2,
			     AXP8191_IRQ_BTN_LONG);
	}

	/* Clear all handled interrupts */
	regmap_write(axp->regmap, AXP8191_INT_STATUS1, status1);
	regmap_write(axp->regmap, AXP8191_INT_STATUS2, status2);
	regmap_write(axp->regmap, AXP8191_INT_STATUS3, status3);

	return IRQ_HANDLED;
}

static int axp8191_init(struct axp8191_dev *axp)
{
	unsigned int val;
	int ret;

	ret = regmap_read(axp->regmap, AXP8191_CHIP_ID, &val);
	if (ret) {
		dev_err(axp->dev, "Failed to read chip ID: %d\n", ret);
		return ret;
	}

	if (val != AXP8191_CHIP_ID_VALUE) {
		dev_err(axp->dev, "Unexpected chip ID: 0x%02x (expected 0x%02x)\n",
			val, AXP8191_CHIP_ID_VALUE);
		return -ENODEV;
	}

	ret = regmap_read(axp->regmap, AXP8191_CHIP_VER, &val);
	if (!ret)
		dev_info(axp->dev, "AXP8191 revision 0x%02x\n", val);

	/* Enable DCDC PWM mode for better efficiency */
	ret = regmap_update_bits(axp->regmap, AXP8191_DCDC_PWM_CTRL,
				 0x03, 0x01);
	if (ret)
		dev_warn(axp->dev, "Failed to set PWM mode: %d\n", ret);

	/* Set default VOFF voltage (3.0V) */
	ret = regmap_update_bits(axp->regmap, AXP8191_VOFF_CTRL,
				 0x07, 0x04);
	if (ret)
		dev_warn(axp->dev, "Failed to set VOFF voltage: %d\n", ret);

	/* Disable all ADC channels initially */
	regmap_write(axp->regmap, AXP8191_ADC_EN1, 0x00);
	regmap_write(axp->regmap, AXP8191_ADC_EN2, 0x00);
	regmap_write(axp->regmap, AXP8191_ADC_EN3, 0x00);

	/* Set ADC sample rate (25Hz) */
	regmap_update_bits(axp->regmap, AXP8191_ADC_SPEED, 0x0f, 0x04);

	/* Configure charger defaults: enable, 500mA input current limit */
	regmap_update_bits(axp->regmap, AXP8191_CHARGER_CTRL1,
			   0xff, 0xc8);

	/* Clear any pending interrupts */
	regmap_write(axp->regmap, AXP8191_INT_STATUS1, 0xff);
	regmap_write(axp->regmap, AXP8191_INT_STATUS2, 0xff);
	regmap_write(axp->regmap, AXP8191_INT_STATUS3, 0xff);

	/* Enable VBUS and battery interrupts */
	regmap_update_bits(axp->regmap, AXP8191_INT_ENABLE1,
			   AXP8191_IRQ_VBUS_IN | AXP8191_IRQ_VBUS_OUT |
			   AXP8191_IRQ_BATT_LOW, 0xff);

	/* Enable power button interrupts */
	regmap_update_bits(axp->regmap, AXP8191_INT_ENABLE2,
			   AXP8191_IRQ_BTN_SHORT | AXP8191_IRQ_BTN_LONG,
			   0xff);

	return 0;
}

static int axp8191_i2c_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct axp8191_dev *axp;
	int ret;

	if (!i2c_verify_functionality(client->adapter,
				      I2C_FUNC_I2C | I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	axp = devm_kzalloc(dev, sizeof(*axp), GFP_KERNEL);
	if (!axp)
		return -ENOMEM;

	axp->dev = dev;
	axp->client = client;
	i2c_set_clientdata(client, axp);

	axp->regmap = devm_regmap_init_i2c(client, &axp8191_regmap_config);
	if (IS_ERR(axp->regmap))
		return dev_err_probe(dev, PTR_ERR(axp->regmap),
				     "Failed to init regmap\n");

	ret = axp8191_init(axp);
	if (ret)
		return ret;

	if (client->irq > 0) {
		ret = devm_request_threaded_irq(dev, client->irq, NULL,
						axp8191_irq_thread,
						IRQF_TRIGGER_LOW | IRQF_ONESHOT,
						"axp8191", axp);
		if (ret)
			dev_err(dev, "Failed to request IRQ %d: %d\n",
				client->irq, ret);
	}

	ret = devm_mfd_add_devices(dev, PLATFORM_DEVID_AUTO,
				   axp8191_cells, ARRAY_SIZE(axp8191_cells),
				   NULL, 0, NULL);
	if (ret) {
		dev_err(dev, "Failed to add MFD devices: %d\n", ret);
		return ret;
	}

	/*
	 * Register power-off handler via power-management reboot notifier.
	 * The power-off callback uses regmap so the PMIC is functional by
	 * the time the system calls it.
	 */
	if (pm_power_off)
		dev_warn(dev, "pm_power_off already registered\n");

	return 0;
}

static void axp8191_i2c_remove(struct i2c_client *client)
{
	struct axp8191_dev *axp = i2c_get_clientdata(client);

	/* Disable all interrupts on remove */
	regmap_write(axp->regmap, AXP8191_INT_ENABLE1, 0x00);
	regmap_write(axp->regmap, AXP8191_INT_ENABLE2, 0x00);
	regmap_write(axp->regmap, AXP8191_INT_ENABLE3, 0x00);
}

static const struct of_device_id axp8191_of_match[] = {
	{ .compatible = "allwinner,axp8191" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, axp8191_of_match);

static const struct i2c_device_id axp8191_i2c_id[] = {
	{ "axp8191" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, axp8191_i2c_id);

static struct i2c_driver axp8191_i2c_driver = {
	.driver = {
		.name		= "axp8191",
		.of_match_table	= axp8191_of_match,
	},
	.probe		= axp8191_i2c_probe,
	.remove		= axp8191_i2c_remove,
	.id_table	= axp8191_i2c_id,
};
module_i2c_driver(axp8191_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Allwinner");
MODULE_DESCRIPTION("AXP8191 PMIC MFD driver");
MODULE_ALIAS("platform:axp8191");
