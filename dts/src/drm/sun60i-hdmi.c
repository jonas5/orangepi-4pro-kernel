// SPDX-License-Identifier: GPL-2.0-only
/*
 * Allwinner A733 (sun60iw2) HDMI 2.0 controller driver
 *
 * Copyright (C) 2026 Allwinner Technology Co., Ltd.
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/slab.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_device.h>
#include <drm/drm_edid.h>
#include <drm/drm_probe_helper.h>
#include <drm/display/drm_hdmi_helper.h>
#include <drm/display/drm_scdc_helper.h>
#include <media/cec.h>
#include <media/cec-notifier.h>

/* HDMI controller registers */
#define HDMI_CTRL			0x0000
#define HDMI_STATUS			0x0004
#define HDMI_INTERRUPT_MASK		0x000c
#define HDMI_INTERRUPT_STATUS		0x0010
#define HDMI_HPD_STATUS			0x0020

/* HDMI configuration registers */
#define HDMI_CFG_VIDEO			0x0100
#define HDMI_CFG_AUDIO			0x0104
#define HDMI_CFG_AVI			0x0108
#define HDMI_CFG_AUDIO_INFO		0x010c
#define HDMI_CFG_SPD			0x0110
#define HDMI_CFG_GCP			0x0114

/* HDMI PHY registers */
#define HDMI_PHY_BASE			0x0200
#define HDMI_PHY_CTRL			0x0200
#define HDMI_PHY_PLL_CTRL		0x0204
#define HDMI_PHY_PLL_STATUS		0x0208
#define HDMI_PHY_TX_CTRL		0x020c
#define HDMI_PHY_TX_STATUS		0x0210

/* HDMI I2S audio registers */
#define HDMI_AUDIO_CTRL			0x0300
#define HDMI_AUDIO_STATUS		0x0304
#define HDMI_AUDIO_SAMPLE_RATE		0x0308
#define HDMI_AUDIO_CHANNELS		0x030c
#define HDMI_AUDIO_CTS_N			0x0310

/* HDMI CEC registers */
#define HDMI_CEC_CTRL			0x0400
#define HDMI_CEC_STATUS			0x0404
#define HDMI_CEC_TX_DATA		0x0408
#define HDMI_CEC_RX_DATA		0x040c
#define HDMI_CEC_INT_MASK		0x0410

/* HDMI control bits */
#define HDMI_CTRL_ENABLE		BIT(31)
#define HDMI_CTRL_HPD			BIT(8)
#define HDMI_CTRL_HDCP_EN		BIT(4)

#define HDMI_STATUS_PLL_LOCKED		BIT(31)
#define HDMI_STATUS_TX_READY		BIT(30)

/* HDMI interrupt bits */
#define HDMI_IRQ_HPD			BIT(0)
#define HDMI_IRQ_AUDIO_FIFO		BIT(4)
#define HDMI_IRQ_CEC			BIT(8)

struct sun60i_hdmi {
	struct device		*dev;
	struct drm_device	*drm;
	struct drm_connector	*connector;
	void __iomem		*regs;

	struct clk		*bus_clk;
	struct clk		*mod_clk;
	struct clk		*pll_clk;
	struct reset_control	*rstc;

	bool			hpd;
	bool			enabled;

	struct cec_adapter	*cec_adap;
	struct cec_notifier	*cec_notifier;

	/* EDID */
	u8			edid[EDID_SIZE];
	bool			edid_valid;
};

struct sun60i_hdmi_mode {
	u32	vic;
	u32	tmds_clock;
	u8遢	vic_samsung;
};

static const struct sun60i_hdmi_mode sun60i_hdmi_modes[] = {
	{ 1,  25200,   0 },	/* 640x480@60Hz */
	{ 2,  27000,   0 },	/* 480p@60Hz */
	{ 3,  27000,   0 },	/* 480p@60Hz */
	{ 4,  74250,   0 },	/* 720p@60Hz */
	{ 5,  74250,   0 },	/* 1080i@60Hz */
	{ 6,  27000,   0 },	/* 480i@60Hz */
	{ 7,  27000,   0 },	/* 480i@60Hz */
	{ 16, 74250,   0 },	/* 1080p@60Hz */
	{ 97, 74250,   0 },	/* 720p@50Hz */
	{ 98, 148500,  0 },	/* 1080p@50Hz */
	{ 102, 148500, 0 },	/* 1080p@24Hz */
	{ 103, 148500, 0 },	/* 1080p@25Hz */
	{ 104, 148500, 0 },	/* 1080p@30Hz */
	{ 93,  594000, 0 },	/* 4K@24Hz */
	{ 94,  594000, 0 },	/* 4K@25Hz */
	{ 95,  594000, 0 },	/* 4K@30Hz */
	{ 96,  594000, 0 },	/* 4K@50Hz */
	{ 97,  594000, 0 },	/* 4K@60Hz */
};

/* AVI InfoFrame color space */
#define HDMI_AVI_CS_RGB		0
#define HDMI_AVI_CS_YCBCR422	1
#define HDMI_AVI_CS_YCBCR444	2

static void sun60i_hdmi_write(struct sun60i_hdmi *hdmi, u32 reg, u32 val)
{
	writel(val, hdmi->regs + reg);
}

static u32 sun60i_hdmi_read(struct sun60i_hdmi *hdmi, u32 reg)
{
	return readl(hdmi->regs + reg);
}

static void sun60i_hdmi_enable(struct sun60i_hdmi *hdmi)
{
	sun60i_hdmi_write(hdmi, HDMI_CTRL, HDMI_CTRL_ENABLE);
	hdmi->enabled = true;
}

static void sun60i_hdmi_disable(struct sun60i_hdmi *hdmi)
{
	sun60i_hdmi_write(hdmi, HDMI_CTRL, 0);
	hdmi->enabled = false;
}

static int sun60i_hdmi_phy_init(struct sun60i_hdmi *hdmi, unsigned long tmds)
{
	u32 val;
	int retry = 50;

	sun60i_hdmi_write(hdmi, HDMI_PHY_CTRL, 0x01);
	udelay(100);

	if (tmds > 300000)
		val = 0x8000000f;
	else if (tmds > 150000)
		val = 0x8000000b;
	else
		val = 0x80000007;

	sun60i_hdmi_write(hdmi, HDMI_PHY_CTRL, val);

	do {
		val = sun60i_hdmi_read(hdmi, HDMI_PHY_PLL_STATUS);
		if (val & HDMI_STATUS_PLL_LOCKED)
			return 0;
		udelay(100);
	} while (--retry);

	return -ETIMEDOUT;
}

static void sun60i_hdmi_phy_set_voltage(struct sun60i_hdmi *hdmi,
					unsigned int mv)
{
	u32 val;

	val = sun60i_hdmi_read(hdmi, HDMI_PHY_TX_CTRL);
	val &= ~0xff;
	val |= (mv / 10) & 0xff;
	sun60i_hdmi_write(hdmi, HDMI_PHY_TX_CTRL, val);
}

static void sun60i_hdmi_set_video_mode(struct sun60i_hdmi *hdmi,
				       const struct drm_display_mode *mode)
{
	u32 val;

	val = 0;
	if (mode->flags & DRM_MODE_FLAG_PHSYNC)
		val |= BIT(4);
	if (mode->flags & DRM_MODE_FLAG_PVSYNC)
		val |= BIT(5);
	sun60i_hdmi_write(hdmi, HDMI_CFG_VIDEO, val);
}

static void sun60i_hdmi_set_audio(struct sun60i_hdmi *hdmi,
				  unsigned int sample_rate,
				  unsigned int channels)
{
	u32 val;

	val = 0x00000001;  /* Audio enable */
	sun60i_hdmi_write(hdmi, HDMI_AUDIO_CTRL, val);

	val = (channels - 1) << 16;
	sun60i_hdmi_write(hdmi, HDMI_AUDIO_CHANNELS, val);

	switch (sample_rate) {
	case 32000:
		val = 0x03;
		break;
	case 44100:
		val = 0x00;
		break;
	case 48000:
		val = 0x02;
		break;
	default:
		val = 0x02;
		break;
	}

	sun60i_hdmi_write(hdmi, HDMI_AUDIO_SAMPLE_RATE, val);
}

static void sun60i_hdmi_set_cts_n(struct sun60i_hdmi *hdmi,
				  unsigned int tmds,
				  unsigned int sample_rate)
{
	u32 n, cts;

	switch (sample_rate) {
	case 32000:
		n = 4096;
		break;
	case 44100:
		n = 6272;
		break;
	case 48000:
		n = 6144;
		break;
	default:
		n = 6144;
		break;
	}

	cts = (tmds / 100) * n / 128;

	sun60i_hdmi_write(hdmi, HDMI_AUDIO_CTS_N,
			  (cts << 12) | (n & 0xfff));
}

static void sun60i_hdmi_send_avi_infoframe(struct sun60i_hdmi *hdmi,
					   const struct drm_display_mode *mode,
					   int color_space)
{
	u8 avi[16];
	u32 val;

	memset(avi, 0, sizeof(avi));

	avi[0] = 0x82;
	avi[1] = 0x02;
	avi[2] = 0x0d;
	avi[3] = (color_space & 0x03);
	avi[4] = 0;

	avi[5] = mode->vdisplay & 0xff;

	avi[7] = 0;

	val = 0;
	val |= (avi[3] << 0);
	val |= (avi[4] << 8);
	val |= (avi[5] << 16);
	val |= (avi[7] << 24);
	sun60i_hdmi_write(hdmi, HDMI_CFG_AVI, val);
}

static void sun60i_hdmi_set_color_space(struct sun60i_hdmi *hdmi,
					int color_space)
{
	u32 val;

	val = sun60i_hdmi_read(hdmi, HDMI_CFG_GCP);
	val &= ~0x03;
	val |= color_space;
	sun60i_hdmi_write(hdmi, HDMI_CFG_GCP, val);
}

/*
 * HPD handling
 */

static bool sun60i_hdmi_hPD(struct sun60i_hdmi *hdmi)
{
	u32 val;

	val = sun60i_hdmi_read(hdmi, HDMI_HPD_STATUS);
	return !!(val & HDMI_CTRL_HPD);
}

static irqreturn_t sun60i_hdmi_isr(int irq, void *data)
{
	struct sun60i_hdmi *hdmi = data;
	u32 status;

	status = sun60i_hdmi_read(hdmi, HDMI_INTERRUPT_STATUS);
	if (!status)
		return IRQ_NONE;

	sun60i_hdmi_write(hdmi, HDMI_INTERRUPT_STATUS, status);

	if (status & HDMI_IRQ_HPD) {
		bool hpd = sun60i_hdmi_hPD(hdmi);

		if (hpd != hdmi->hpd) {
			hdmi->hpd = hpd;
			if (hdmi->connector)
				drm_connector_helper_hpd_irq_event(hdmi->connector);
		}
	}

	if (status & HDMI_IRQ_CEC)
		cec_received_msg(hdmi->cec_adap, NULL);

	return IRQ_HANDLED;
}

/*
 * Connector
 */

static int sun60i_hdmi_connector_get_modes(struct drm_connector *connector)
{
	struct sun60i_hdmi *hdmi =
		container_of(connector, struct sun60i_hdmi, connector);
	int count;

	if (!hdmi->edid_valid) {
		count = drm_connector_helper_get_modes(connector);
		return count;
	}

	count = drm_edid_connector_add_modes(connector);

	return count;
}

static const struct drm_connector_helper_funcs sun60i_hdmi_conn_helpers = {
	.get_modes	= sun60i_hdmi_connector_get_modes,
};

static const struct drm_connector_funcs sun60i_hdmi_conn_funcs = {
	.reset			= drm_atomic_helper_connector_reset,
	.destroy		= drm_connector_cleanup,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
};

/*
 * CEC
 */

static int sun60i_hdmi_cec_adap_enable(struct cec_adapter *adap, bool enable)
{
	struct sun60i_hdmi *hdmi = cec_get_drvdata(adap);
	u32 val;

	val = enable ? HDMI_IRQ_CEC : 0;
	sun60i_hdmi_write(hdmi, HDMI_CEC_INT_MASK, val);

	val = enable ? 0x01 : 0x00;
	sun60i_hdmi_write(hdmi, HDMI_CEC_CTRL, val);

	return 0;
}

static int sun60i_hdmi_cec_adap_transmit(struct cec_adapter *adap,
					 u8 attempts, u32 signal_free_time,
					 struct cec_msg *msg)
{
	struct sun60i_hdmi *hdmi = cec_get_drvdata(adap);
	int i;

	if (msg->len > 16)
		return -EINVAL;

	for (i = 0; i < msg->len; i++)
		sun60i_hdmi_write(hdmi, HDMI_CEC_TX_DATA + i * 4, msg->msg[i]);

	sun60i_hdmi_write(hdmi, HDMI_CEC_CTRL, 0x03);

	return 0;
}

static const struct cec_adap_ops sun60i_hdmi_cec_ops = {
	.adap_enable	= sun60i_hdmi_cec_adap_enable,
	.adap_transmit	= sun60i_hdmi_cec_adap_transmit,
};

/*
 * Clock management
 */

static int sun60i_hdmi_clks_init(struct sun60i_hdmi *hdmi)
{
	int ret;

	hdmi->bus_clk = devm_clk_get(hdmi->dev, "bus");
	if (IS_ERR(hdmi->bus_clk))
		return dev_err_probe(hdmi->dev, PTR_ERR(hdmi->bus_clk),
				     "failed to get bus clock\n");

	hdmi->mod_clk = devm_clk_get(hdmi->dev, "mod");
	if (IS_ERR(hdmi->mod_clk))
		return dev_err_probe(hdmi->dev, PTR_ERR(hdmi->mod_clk),
				     "failed to get mod clock\n");

	hdmi->pll_clk = devm_clk_get(hdmi->dev, "pll");
	if (IS_ERR(hdmi->pll_clk))
		return dev_err_probe(hdmi->dev, PTR_ERR(hdmi->pll_clk),
				     "failed to get PLL clock\n");

	ret = clk_set_rate(hdmi->mod_clk, 297000000);
	if (ret)
		dev_warn(hdmi->dev, "failed to set mod clock rate: %d\n", ret);

	return 0;
}

static int sun60i_hdmi_hw_init(struct sun60i_hdmi *hdmi)
{
	int ret;

	ret = reset_control_deassert(hdmi->rstc);
	if (ret) {
		dev_err(hdmi->dev, "failed to deassert reset: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(hdmi->bus_clk);
	if (ret)
		goto err_rst_assert;

	ret = clk_prepare_enable(hdmi->mod_clk);
	if (ret)
		goto err_bus_clk;

	ret = clk_prepare_enable(hdmi->pll_clk);
	if (ret)
		goto err_mod_clk;

	return 0;

err_mod_clk:
	clk_disable_unprepare(hdmi->mod_clk);
err_bus_clk:
	clk_disable_unprepare(hdmi->bus_clk);
err_rst_assert:
	reset_control_assert(hdmi->rstc);

	return ret;
}

/*
 * Component bind/unbind
 */

static int sun60i_hdmi_bind(struct device *dev, struct device *master,
			    void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sun60i_hdmi *hdmi;
	int irq, ret;

	hdmi = devm_kzalloc(dev, sizeof(*hdmi), GFP_KERNEL);
	if (!hdmi)
		return -ENOMEM;

	dev_set_drvdata(dev, hdmi);
	hdmi->dev = dev;

	ret = sun60i_hdmi_clks_init(hdmi);
	if (ret)
		return ret;

	hdmi->rstc = devm_reset_control_get_shared(dev, NULL);
	if (IS_ERR(hdmi->rstc))
		return dev_err_probe(dev, PTR_ERR(hdmi->rstc),
				     "failed to get reset control\n");

	ret = sun60i_hdmi_hw_init(hdmi);
	if (ret)
		return ret;

	hdmi->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(hdmi->regs)) {
		ret = PTR_ERR(hdmi->regs);
		goto err_clk_disable;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto err_clk_disable;
	}

	ret = devm_request_irq(dev, irq, sun60i_hdmi_isr, 0,
			       dev_name(dev), hdmi);
	if (ret) {
		dev_err(dev, "failed to request IRQ %d: %d\n", irq, ret);
		goto err_clk_disable;
	}

	sun60i_hdmi_enable(hdmi);

	/* Enable HPD interrupt */
	sun60i_hdmi_write(hdmi, HDMI_INTERRUPT_MASK, HDMI_IRQ_HPD);

	hdmi->hpd = sun60i_hdmi_hPD(hdmi);

	return 0;

err_clk_disable:
	clk_disable_unprepare(hdmi->pll_clk);
	clk_disable_unprepare(hdmi->mod_clk);
	clk_disable_unprepare(hdmi->bus_clk);
	reset_control_assert(hdmi->rstc);

	return ret;
}

static void sun60i_hdmi_unbind(struct device *dev, struct device *master,
			       void *data)
{
	struct sun60i_hdmi *hdmi = dev_get_drvdata(dev);

	sun60i_hdmi_disable(hdmi);

	cec_notifier_conn_unregister(hdmi->cec_notifier);
	cec_unregister_adapter(hdmi->cec_adap);

	clk_disable_unprepare(hdmi->pll_clk);
	clk_disable_unprepare(hdmi->mod_clk);
	clk_disable_unprepare(hdmi->bus_clk);
	reset_control_assert(hdmi->rstc);
}

static const struct component_ops sun60i_hdmi_ops = {
	.bind	= sun60i_hdmi_bind,
	.unbind	= sun60i_hdmi_unbind,
};

static int sun60i_hdmi_probe(struct platform_device *pdev)
{
	return component_add(&pdev->dev, &sun60i_hdmi_ops);
}

static void sun60i_hdmi_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &sun60i_hdmi_ops);
}

static const struct of_device_id sun60i_hdmi_match[] = {
	{ .compatible = "allwinner,sun60i-a733-hdmi" },
	{ }
};
MODULE_DEVICE_TABLE(of, sun60i_hdmi_match);

struct platform_driver sun60i_hdmi_platform_driver = {
	.probe	= sun60i_hdmi_probe,
	.remove	= sun60i_hdmi_remove,
	.driver	= {
		.name		= "sun60i-hdmi",
		.of_match_table	= sun60i_hdmi_match,
	},
};
module_platform_driver(sun60i_hdmi_platform_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Allwinner Technology Co., Ltd.");
MODULE_DESCRIPTION("Allwinner A733 HDMI 2.0 controller driver");
