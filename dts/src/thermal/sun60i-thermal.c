// SPDX-License-Identifier: GPL-2.0-only
/*
 * Allwinner A733 (sun60iw2) Temperature Sensor (THS) driver
 *
 * Copyright (C) 2026 Allwinner Technology Co., Ltd.
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/thermal.h>

/* THS register offsets */
#define THS_CTRL			0x0000
#define THS_INT_EN			0x0004
#define THS_INT_STA			0x0008
#define THS_INT_CTRL			0x000c
#define THS_MBUS_RST			0x0010
#define THS_THS_DATA_0			0x0020
#define THS_THS_DATA_1			0x0024

/* Sensor calibration registers */
#define THS_CALI_CTRL			0x0040
#define THS_CALI_DATA_0			0x0060
#define THS_CALI_DATA_1			0x0064

/* Threshold registers */
#define THS_THRESHOLD_0			0x0080
#define THS_THRESHOLD_1			0x0084
#define THS_THRESHOLD_CTRL		0x0088

/* Sensor control */
#define THS_SENSOR_CTRL			0x00a0
#define THS_SENSOR_EN			0x00a4

/* Interrupt status bits */
#define THS_IRQ_DATA_READY_0		BIT(0)
#define THS_IRQ_DATA_READY_1		BIT(1)
#define THS_IRQ_THRESHOLD_0		BIT(8)
#define THS_IRQ_THRESHOLD_1		BIT(9)
#define THS_IRQ_OVERFLOW_0		BIT(16)
#define THS_IRQ_OVERFLOW_1		BIT(17)

/* Control bits */
#define THS_CTRL_ENABLE			BIT(0)
#define THS_CTRL_SENSOR_EN_0		BIT(8)
#define THS_CTRL_SENSOR_EN_1		BIT(9)
#define THS_CTRLADC_SRC(n)		((n) << 20)

/* Sensor filter */
#define THS_FILTER_EN			BIT(0)
#define THS_FILTER_SHIFT		4
#define THS_FILTER_MASK			(0xf << THS_FILTER_SHIFT)

#define SUN60I_THERMAL_NUM_SENSORS	2
#define SUN60I_THERMAL_AVG_COUNT	16

struct sun60i_thermal_sense {
	void __iomem		*data_reg;
	u32			threshold;
	bool			irq_enabled;
};

struct sun60i_thermal {
	struct device		*dev;
	void __iomem		*regs;
	struct clk		*bus_clk;
	struct clk		*mod_clk;
	struct reset_control	*rstc;
	struct regmap		*regmap;

	struct thermal_zone_device *tzd[SUN60I_THERMAL_NUM_SENSORS];
	struct sun60i_thermal_sense sense[SUN60I_THERMAL_NUM_SENSORS];

	/* Calibration data */
	u32			cali_data[SUN60I_THERMAL_NUM_SENSORS];
	bool			calibrated;
};

static int sun60i_thermal_read_temp(struct sun60i_thermal *ths, int sensor)
{
	u32 reg;

	if (sensor >= SUN60I_THERMAL_NUM_SENSORS)
		return -EINVAL;

	reg = readl(ths->regs + THS_THS_DATA_0 + sensor * 4);
	reg >>= 16;
	reg &= 0xfff;

	return reg;
}

static int sun60i_thermal_zone_get_temp(void *data, int *temp)
{
	struct sun60i_thermal *ths = data;
	struct thermal_zone_device *tz;
	int sensor, raw_temp, cal_temp;

	for (sensor = 0; sensor < SUN60I_THERMAL_NUM_SENSORS; sensor++) {
		tz = ths->tzd[sensor];
		if (!tz)
			continue;

		raw_temp = sun60i_thermal_read_temp(ths, sensor);

		/*
		 * Convert raw sensor value to temperature in millidegrees.
		 * Formula: temp = (raw * 165000 - cal * 165000) >> 12
		 * The sensor has a resolution of about 0.4C per step.
		 */
		if (ths->calibrated)
			cal_temp = (raw_temp - ths->cali_data[sensor]) * 412;
		else
			cal_temp = raw_temp * 412 - 1089000;

		thermal_zone_set_trip_points(tz);
	}

	return 0;
}

static int sun60i_thermal_get_temp(struct thermal_zone_device *tz)
{
	struct sun60i_thermal *ths = thermal_zone_device_priv(tz);
	int sensor;
	u32 raw_temp;
	int temp;

	sensor = (int)thermal_zone_device_get_drvdata(tz);

	raw_temp = sun60i_thermal_read_temp(ths, sensor);

	if (ths->calibrated)
		temp = (raw_temp - ths->cali_data[sensor]) * 412;
	else
		temp = raw_temp * 412 - 1089000;

	return temp;
}

static const struct thermal_zone_device_ops sun60i_thermal_ops = {
	.get_temp	= sun60i_thermal_get_temp,
};

static void sun60i_thermal_set_threshold(struct sun60i_thermal *ths,
					 int sensor, u32 temp)
{
	u32 reg;

	reg = readl(ths->regs + THS_THRESHOLD_CTRL);

	if (temp > 0) {
		writel(temp, ths->regs + THS_THRESHOLD_0 + sensor * 4);
		reg |= BIT(sensor);
	} else {
		reg &= ~BIT(sensor);
	}

	writel(reg, ths->regs + THS_THRESHOLD_CTRL);
}

static irqreturn_t sun60i_thermal_isr(int irq, void *data)
{
	struct sun60i_thermal *ths = data;
	u32 status;
	int i;

	status = readl(ths->regs + THS_INT_STA);
	if (!status)
		return IRQ_NONE;

	writel(status, ths->regs + THS_INT_STA);

	for (i = 0; i < SUN60I_THERMAL_NUM_SENSORS; i++) {
		if (status & (THS_IRQ_THRESHOLD_0 << i))
			thermal_zone_device_update(ths->tzd[i],
						   THERMAL_EVENT_UNSPECIFIED);
	}

	return IRQ_HANDLED;
}

static void sun60i_thermal_init_sensors(struct sun60i_thermal *ths)
{
	u32 val;

	val = readl(ths->regs + THS_CTRL);
	val |= THS_CTRL_ENABLE;
	val |= THS_CTRL_SENSOR_EN_0 | THS_CTRL_SENSOR_EN_1;
	val |= THS_CTRLADC_SRC(0);
	writel(val, ths->regs + THS_CTRL);

	val = THS_FILTER_EN | (0x07 << THS_FILTER_SHIFT);
	writel(val, ths->regs + THS_MBUS_RST);

	val = THS_INT_STA;
	writel(val, ths->regs + THS_INT_STA);

	val = THS_IRQ_DATA_READY_0 | THS_IRQ_DATA_READY_1;
	writel(val, ths->regs + THS_INT_EN);

	sun60i_thermal_set_threshold(ths, 0, 95);
	sun60i_thermal_set_threshold(ths, 1, 95);
}

static int sun60i_thermal_calibrate(struct sun60i_thermal *ths)
{
	u32 val;

	val = readl(ths->regs + THS_CALI_CTRL);
	if (val & BIT(31)) {
		ths->cali_data[0] = readl(ths->regs + THS_CALI_DATA_0);
		ths->cali_data[1] = readl(ths->regs + THS_CALI_DATA_1);
		ths->calibrated = true;
	} else {
		ths->calibrated = false;
	}

	return 0;
}

static int sun60i_thermal_clks_init(struct sun60i_thermal *ths)
{
	int ret;

	ths->bus_clk = devm_clk_get(ths->dev, "bus");
	if (IS_ERR(ths->bus_clk))
		return dev_err_probe(ths->dev, PTR_ERR(ths->bus_clk),
				     "failed to get bus clock\n");

	ths->mod_clk = devm_clk_get(ths->dev, "mod");
	if (IS_ERR(ths->mod_clk))
		return dev_err_probe(ths->dev, PTR_ERR(ths->mod_clk),
				     "failed to get mod clock\n");

	ret = clk_set_rate(ths->mod_clk, 24000000);
	if (ret)
		dev_warn(ths->dev, "failed to set mod clock rate: %d\n", ret);

	return 0;
}

static int sun60i_thermal_hw_init(struct sun60i_thermal *ths)
{
	int ret;

	ret = reset_control_deassert(ths->rstc);
	if (ret) {
		dev_err(ths->dev, "failed to deassert reset: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(ths->bus_clk);
	if (ret) {
		dev_err(ths->dev, "failed to enable bus clock: %d\n", ret);
		goto err_rst_assert;
	}

	ret = clk_prepare_enable(ths->mod_clk);
	if (ret) {
		dev_err(ths->dev, "failed to enable mod clock: %d\n", ret);
		goto err_bus_clk;
	}

	return 0;

err_bus_clk:
	clk_disable_unprepare(ths->bus_clk);
err_rst_assert:
	reset_control_assert(ths->rstc);

	return ret;
}

static int sun60i_thermal_probe(struct platform_device *pdev)
{
	struct sun60i_thermal *ths;
	struct device *dev = &pdev->dev;
	int irq, ret;
	int i;

	ths = devm_kzalloc(dev, sizeof(*ths), GFP_KERNEL);
	if (!ths)
		return -ENOMEM;

	dev_set_drvdata(dev, ths);
	ths->dev = dev;

	ret = sun60i_thermal_clks_init(ths);
	if (ret)
		return ret;

	ths->rstc = devm_reset_control_get_shared(dev, NULL);
	if (IS_ERR(ths->rstc))
		return dev_err_probe(dev, PTR_ERR(ths->rstc),
				     "failed to get reset control\n");

	ret = sun60i_thermal_hw_init(ths);
	if (ret)
		return ret;

	ths->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(ths->regs)) {
		ret = PTR_ERR(ths->regs);
		goto err_clk_disable;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq >= 0) {
		ret = devm_request_irq(dev, irq, sun60i_thermal_isr, 0,
				       dev_name(dev), ths);
		if (ret) {
			dev_err(dev, "failed to request IRQ %d: %d\n", irq, ret);
			goto err_clk_disable;
		}
	}

	sun60i_thermal_calibrate(ths);
	sun60i_thermal_init_sensors(ths);

	for (i = 0; i < SUN60I_THERMAL_NUM_SENSORS; i++) {
		ths->tzd[i] = devm_thermal_zone_of_sensor_register(dev, i,
					ths, &sun60i_thermal_ops);
		if (IS_ERR(ths->tzd[i])) {
			ret = PTR_ERR(ths->tzd[i]);
			dev_err(dev, "failed to register thermal zone %d: %d\n",
				i, ret);
			goto err_clk_disable;
		}
	}

	return 0;

err_clk_disable:
	clk_disable_unprepare(ths->mod_clk);
	clk_disable_unprepare(ths->bus_clk);
	reset_control_assert(ths->rstc);

	return ret;
}

static void sun60i_thermal_remove(struct platform_device *pdev)
{
	struct sun60i_thermal *ths = platform_get_drvdata(pdev);

	sun60i_thermal_set_threshold(ths, 0, 0);
	sun60i_thermal_set_threshold(ths, 1, 0);

	writel(0, ths->regs + THS_INT_EN);
	writel(0, ths->regs + THS_CTRL);

	clk_disable_unprepare(ths->mod_clk);
	clk_disable_unprepare(ths->bus_clk);
	reset_control_assert(ths->rstc);
}

static const struct of_device_id sun60i_thermal_match[] = {
	{ .compatible = "allwinner,sun60i-a733-thermal" },
	{ }
};
MODULE_DEVICE_TABLE(of, sun60i_thermal_match);

struct platform_driver sun60i_thermal_platform_driver = {
	.probe	= sun60i_thermal_probe,
	.remove	= sun60i_thermal_remove,
	.driver	= {
		.name		= "sun60i-thermal",
		.of_match_table	= sun60i_thermal_match,
	},
};
module_platform_driver(sun60i_thermal_platform_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Allwinner Technology Co., Ltd.");
MODULE_DESCRIPTION("Allwinner A733 Temperature Sensor driver");
