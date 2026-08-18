// SPDX-License-Identifier: GPL-2.0-only
/*
 * Allwinner A733 (sun60iw2) Timing Controller (TCON) driver
 *
 * Copyright (C) 2026 Allwinner Technology Co., Ltd.
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/slab.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_device.h>
#include <drm/drm_modes.h>
#include <drm/drm_probe_helper.h>

/* TCON register offsets */
#define TCON_GLB_CTL			0x0000
#define TCON_GLB_STATUS		0x0004
#define TCON_INT_CFG			0x0030
#define TCON_INT_FLAG			0x0034
#define TCON_DAT_RSTY			0x0048
#define TCON1_DCLK_REV			0x0040

/* TCON0 timing registers */
#define TCON0_CTRL			0x0100
#define TCON0_DCLK			0x0104
#define TCON0_BASIC_TIMING0		0x0108
#define TCON0_BASIC_TIMING1		0x010c
#define TCON0_BASIC_TIMING2		0x0110
#define TCON0_HV_IF			0x0114
#define TCON0_SPATAG			0x0118
#define TCON0_TIMING			0x0130
#define TCON0_TIMING_SYNC		0x0134

/* TCON1 timing registers */
#define TCON1_CTRL			0x0200
#define TCON1_DCLK			0x0204
#define TCON1_BASIC_TIMING0		0x0208
#define TCON1_BASIC_TIMING1		0x020c
#define TCON1_BASIC_TIMING2		0x0210
#define TCON1_HV_IF			0x0214
#define TCON1_TIMING			0x0230
#define TCON1_TIMING_SYNC		0x0234

/* LVDS registers */
#define TCON1_LVDS_IF			0x0220
#define TCON1_LVDS_POL			0x0224
#define TCON1_LVDS_CLK			0x0228
#define TCON1_LVDS_DATA0		0x0260
#define TCON1_LVDS_DATA1		0x0264
#define TCON1_LVDS_DATA2		0x0268
#define TCON1_LVDS_DATA3		0x026c

/* MIPI-DSI registers */
#define TCON1_DSI_IF			0x0220
#define TCON1_DSI_CLK			0x0228

/* Bus width defines */
#define TCON_DSI_BUS_WIDTH_16		0
#define TCON_DSI_BUS_WIDTH_18		1
#define TCON_DSI_BUS_WIDTH_24		2

/* LVDS link modes */
#define TCON_LVDS_LINK_SINGLE		0
#define TCON_LVDS_LINK_DUAL		1

struct sun60i_tcon {
	struct device		*dev;
	struct drm_device	*drm;
	void __iomem		*regs;
	struct clk		*bus_clk;
	struct clk		*tcon_clk;
	struct reset_control	*rstc;

	bool			is_tcon0;
	bool			is_tcon1;
	bool			enabled;
};

static void sun60i_tcon_enable(struct sun60i_tcon *tcon)
{
	u32 val;

	val = readl(tcon->regs + TCON_GLB_CTL);
	val |= BIT(31);  /* Enable TCON */
	writel(val, tcon->regs + TCON_GLB_CTL);

	tcon->enabled = true;
}

static void sun60i_tcon_disable(struct sun60i_tcon *tcon)
{
	u32 val;

	val = readl(tcon->regs + TCON_GLB_CTL);
	val &= ~BIT(31);  /* Disable TCON */
	writel(val, tcon->regs + TCON_GLB_CTL);

	tcon->enabled = false;
}

static void sun60i_tcon0_set_timing(struct sun60i_tcon *tcon,
				    const struct drm_display_mode *mode)
{
	u32 hsync_len, vsync_len;
	u32 hback_porch, vback_porch;
	u32 hfront_porch, vfront_porch;
	u32 val;

	hsync_len = mode->hsync_end - mode->hsync_start;
	vsync_len = mode->vsync_end - mode->vsync_start;
	hback_porch = mode->htotal - mode->hsync_end;
	vback_porch = mode->vtotal - mode->vsync_end;
	hfront_porch = mode->hsync_start - mode->hdisplay;
	vfront_porch = mode->vsync_start - mode->vdisplay;

	/* Horizontal timing: hsync_len[27:16], htotal[11:0] */
	val = ((mode->htotal - 1) & 0xfff);
	val |= ((hsync_len - 1) & 0xfff) << 16;
	writel(val, tcon->regs + TCON0_BASIC_TIMING0);

	/* Vertical timing: vsync_len[27:16], vtotal[11:0] */
	val = ((mode->vtotal - 1) & 0xfff);
	val |= ((vsync_len - 1) & 0xfff) << 16;
	writel(val, tcon->regs + TCON0_BASIC_TIMING1);

	/* Display size: width[28:16], height[12:0] */
	val = ((mode->hdisplay - 1) & 0x1fff);
	val |= ((mode->vdisplay - 1) & 0x1fff) << 16;
	writel(val, tcon->regs + TCON0_BASIC_TIMING2);

	/* H/V sync polarity */
	val = 0;
	if (mode->flags & DRM_MODE_FLAG_PHSYNC)
		val |= BIT(25);
	if (mode->flags & DRM_MODE_FLAG_PVSYNC)
		val |= BIT(24);
	writel(val, tcon->regs + TCON0_TIMING_SYNC);
}

static void sun60i_tcon1_set_timing(struct sun60i_tcon *tcon,
				    const struct drm_display_mode *mode)
{
	u32 hsync_len, vsync_len;
	u32 val;

	hsync_len = mode->hsync_end - mode->hsync_start;
	vsync_len = mode->vsync_end - mode->vsync_start;

	val = ((mode->htotal - 1) & 0xfff);
	val |= ((hsync_len - 1) & 0xfff) << 16;
	writel(val, tcon->regs + TCON1_BASIC_TIMING0);

	val = ((mode->vtotal - 1) & 0xfff);
	val |= ((vsync_len - 1) & 0xfff) << 16;
	writel(val, tcon->regs + TCON1_BASIC_TIMING1);

	val = ((mode->hdisplay - 1) & 0x1fff);
	val |= ((mode->vdisplay - 1) & 0x1fff) << 16;
	writel(val, tcon->regs + TCON1_BASIC_TIMING2);

	val = 0;
	if (mode->flags & DRM_MODE_FLAG_PHSYNC)
		val |= BIT(25);
	if (mode->flags & DRM_MODE_FLAG_PVSYNC)
		val |= BIT(24);
	writel(val, tcon->regs + TCON1_TIMING_SYNC);
}

static void sun60i_tcon0_set_dclk(struct sun60i_tcon *tcon, unsigned long rate)
{
	writel(rate - 1, tcon->regs + TCON0_DCLK);
}

static void sun60i_tcon1_set_dclk(struct sun60i_tcon *tcon, unsigned long rate)
{
	writel(rate - 1, tcon->regs + TCON1_DCLK);
}

static void sun60i_tcon_lvds_enable(struct sun60i_tcon *tcon, bool dual_link)
{
	u32 val;

	val = BIT(31);  /* LVDS enable */
	if (dual_link)
		val |= BIT(30);

	writel(val, tcon->regs + TCON1_LVDS_IF);
	writel(0x03, tcon->regs + TCON1_LVDS_CLK);
}

static void sun60i_tcon_lvds_disable(struct sun60i_tcon *tcon)
{
	writel(0, tcon->regs + TCON1_LVDS_IF);
}

static void sun60i_tcon_dsi_enable(struct sun60i_tcon *tcon)
{
	u32 val;

	val = BIT(31);  /* DSI interface enable */
	val |= TCON_DSI_BUS_WIDTH_24 << 12;
	writel(val, tcon->regs + TCON1_DSI_IF);
}

static void sun60i_tcon_dsi_disable(struct sun60i_tcon *tcon)
{
	writel(0, tcon->regs + TCON1_DSI_IF);
}

static void sun60i_tcon_hDMI_enable(struct sun60i_tcon *tcon)
{
	u32 val;

	val = readl(tcon->regs + TCON0_CTRL);
	val |= BIT(31);  /* TCON0 enable */
	val |= BIT(25);  /* HDMI output mode */
	writel(val, tcon->regs + TCON0_CTRL);
}

static void sun60i_tcon_hDMI_disable(struct sun60i_tcon *tcon)
{
	u32 val;

	val = readl(tcon->regs + TCON0_CTRL);
	val &= ~(BIT(31) | BIT(25));
	writel(val, tcon->regs + TCON0_CTRL);
}

/*
 * IRQ handling
 */

static irqreturn_t sun60i_tcon_isr(int irq, void *data)
{
	struct sun60i_tcon *tcon = data;
	u32 status;

	status = readl(tcon->regs + TCON_INT_FLAG);
	if (!status)
		return IRQ_NONE;

	writel(status, tcon->regs + TCON_INT_FLAG);

	return IRQ_HANDLED;
}

/*
 * Clock management
 */

static int sun60i_tcon_clks_init(struct sun60i_tcon *tcon)
{
	int ret;

	tcon->bus_clk = devm_clk_get(tcon->dev, "bus");
	if (IS_ERR(tcon->bus_clk))
		return dev_err_probe(tcon->dev, PTR_ERR(tcon->bus_clk),
				     "failed to get bus clock\n");

	tcon->tcon_clk = devm_clk_get(tcon->dev, "tcon");
	if (IS_ERR(tcon->tcon_clk))
		return dev_err_probe(tcon->dev, PTR_ERR(tcon->tcon_clk),
				     "failed to get TCON clock\n");

	ret = clk_set_rate(tcon->tcon_clk, 216000000);
	if (ret)
		return dev_err_probe(tcon->dev, ret,
				     "failed to set TCON clock rate\n");

	return 0;
}

static int sun60i_tcon_hw_init(struct sun60i_tcon *tcon)
{
	int ret;

	ret = reset_control_deassert(tcon->rstc);
	if (ret) {
		dev_err(tcon->dev, "failed to deassert reset: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(tcon->bus_clk);
	if (ret) {
		dev_err(tcon->dev, "failed to enable bus clock: %d\n", ret);
		goto err_rst_assert;
	}

	ret = clk_prepare_enable(tcon->tcon_clk);
	if (ret) {
		dev_err(tcon->dev, "failed to enable TCON clock: %d\n", ret);
		goto err_bus_clk;
	}

	return 0;

err_bus_clk:
	clk_disable_unprepare(tcon->bus_clk);
err_rst_assert:
	reset_control_assert(tcon->rstc);

	return ret;
}

/*
 * Component bind/unbind
 */

static int sun60i_tcon_bind(struct device *dev, struct device *master,
			    void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sun60i_tcon *tcon;
	int irq, ret;

	tcon = devm_kzalloc(dev, sizeof(*tcon), GFP_KERNEL);
	if (!tcon)
		return -ENOMEM;

	dev_set_drvdata(dev, tcon);
	tcon->dev = dev;

	ret = sun60i_tcon_clks_init(tcon);
	if (ret)
		return ret;

	tcon->rstc = devm_reset_control_get_shared(dev, NULL);
	if (IS_ERR(tcon->rstc))
		return dev_err_probe(dev, PTR_ERR(tcon->rstc),
				     "failed to get reset control\n");

	ret = sun60i_tcon_hw_init(tcon);
	if (ret)
		return ret;

	tcon->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(tcon->regs)) {
		ret = PTR_ERR(tcon->regs);
		goto err_clk_disable;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq >= 0) {
		ret = devm_request_irq(dev, irq, sun60i_tcon_isr, 0,
				       dev_name(dev), tcon);
		if (ret) {
			dev_err(dev, "failed to request IRQ %d: %d\n", irq, ret);
			goto err_clk_disable;
		}
	}

	sun60i_tcon_enable(tcon);

	return 0;

err_clk_disable:
	clk_disable_unprepare(tcon->tcon_clk);
	clk_disable_unprepare(tcon->bus_clk);
	reset_control_assert(tcon->rstc);

	return ret;
}

static void sun60i_tcon_unbind(struct device *dev, struct device *master,
			       void *data)
{
	struct sun60i_tcon *tcon = dev_get_drvdata(dev);

	sun60i_tcon_disable(tcon);

	clk_disable_unprepare(tcon->tcon_clk);
	clk_disable_unprepare(tcon->bus_clk);
	reset_control_assert(tcon->rstc);
}

static const struct component_ops sun60i_tcon_ops = {
	.bind	= sun60i_tcon_bind,
	.unbind	= sun60i_tcon_unbind,
};

static int sun60i_tcon_probe(struct platform_device *pdev)
{
	return component_add(&pdev->dev, &sun60i_tcon_ops);
}

static void sun60i_tcon_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &sun60i_tcon_ops);
}

static const struct of_device_id sun60i_tcon_match[] = {
	{ .compatible = "allwinner,sun60i-a733-tcon", },
	{ }
};
MODULE_DEVICE_TABLE(of, sun60i_tcon_match);

struct platform_driver sun60i_tcon_platform_driver = {
	.probe	= sun60i_tcon_probe,
	.remove	= sun60i_tcon_remove,
	.driver	= {
		.name		= "sun60i-tcon",
		.of_match_table	= sun60i_tcon_match,
	},
};
module_platform_driver(sun60i_tcon_platform_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Allwinner Technology Co., Ltd.");
MODULE_DESCRIPTION("Allwinner A733 Timing Controller driver");
