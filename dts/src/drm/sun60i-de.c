// SPDX-License-Identifier: GPL-2.0-only
/*
 * Allwinner A733 (sun60iw2) Display Engine 2 driver
 *
 * Copyright (C) 2026 Allwinner Technology Co., Ltd.
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/dma-mapping.h>
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
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem_dma_helper.h>
#include <drm/drm_plane.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_vblank.h>

/* DE register offsets */
#define DE_MUX_DCLK_CFG		0x0010
#define DE_MUX_DEBUG_CFG		0x0018
#define DE_MUX_BLEND_EN			0x0080
#define DE_MUX_BLEND_ROUTE		0x0084
#define DE_MUX_BLEND_SIZE		0x0088
#define DE_MUX_BG_COLOR			0x008c

/* Channel (layer) registers */
#define DE_CH_FORMAT			0x0000
#define DE_CH_SIZE			0x0004
#define DE_CH_COORD			0x0008
#define DE_CH_PITCH			0x000c
#define DE_CH_TOP_LADDR			0x0010
#define DE_CH_COLOR_CTL			0x0014
#define DE_CH_SRC_COLOR_KEY		0x0018
#define DE_CH_DST_COLOR_KEY		0x001c
#define DE_CH_SCALE_CTRL		0x0040
#define DE_CH_SCALE_FACTOR		0x0044
#define DE_CH_SCALE_PHASE		0x0048

/* Alpha blending registers */
#define DE_BLEND_MODE			0x0000
#define DE_BLEND_COLOR			0x0004
#define DE_BLEND_OUT_COLOR		0x0008

/* Display engine architecture definitions */
#define SUN60I_DE_CLK_NUM		4
#define SUN60I_DE_MAX_CHANNELS		5
#define SUN60I_DE_PRIMARY_CHANNEL	4
#define SUN60I_DE_WIDTH_MAX		4096
#define SUN60I_DE_HEIGHT_MAX		4096

/* Layer format definitions */
#define DE_FORMAT_ARGB8888		0
#define DE_FORMAT_XRGB8888		1
#define DE_FORMAT_RGB565		2
#define DE_FORMAT_NV12			8
#define DE_FORMAT_NV21			9
#define DE_FORMAT_YUV420P		10

/* Alpha blending modes */
#define DE_BLEND_MODE_ALPHA		0
#define DE_BLEND_MODE_PREMULTI		1
#define DE_BLEND_MODE_COVERAGE		2
#define DE_BLEND_MODE_FBC		3

#define to_sun60i_de(x)		container_of(x, struct sun60i_de, drm)

struct sun60i_de_channel {
	void __iomem		*regs;
	u8			format;
	bool			enabled;
};

struct sun60i_de {
	struct device		*dev;
	struct drm_device	*drm;
	void __iomem		*regs;
	void __iomem		*mixer_regs;
	struct clk		*bus_clk;
	struct clk		*mod_clk;
	struct clk		*dclk;
	struct reset_control	*rstc;

	struct drm_plane	*planes[SUN60I_DE_MAX_CHANNELS];
	struct sun60i_de_channel channels[SUN60I_DE_MAX_CHANNELS];

	spinlock_t		lock;
};

/* Layer format table */
struct de_format_info {
	u32	drm_format;
	u8	bpp;
	u8	nplanes;
	u8	hsub;
	u8	vsub;
};

static const struct de_format_info de_formats[] = {
	[DE_FORMAT_ARGB8888] = { DRM_FORMAT_ARGB8888, 32, 1, 1, 1 },
	[DE_FORMAT_XRGB8888] = { DRM_FORMAT_XRGB8888, 32, 1, 1, 1 },
	[DE_FORMAT_RGB565]   = { DRM_FORMAT_RGB565,   16, 1, 1, 1 },
	[DE_FORMAT_NV12]     = { DRM_FORMAT_NV12,      8, 2, 2, 2 },
	[DE_FORMAT_NV21]     = { DRM_FORMAT_NV21,      8, 2, 2, 2 },
	[DE_FORMAT_YUV420P]  = { DRM_FORMAT_YUV420,    8, 3, 2, 2 },
};

static int sun60i_de_format_to_index(u32 drm_format)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(de_formats); i++) {
		if (de_formats[i].drm_format == drm_format)
			return i;
	}

	return -EINVAL;
}

static void sun60i_de_channel_enable(struct sun60i_de *de, int channel,
				     bool enable)
{
	u32 val;

	val = readl(de->mixer_regs + DE_MUX_BLEND_EN);

	if (enable)
		val |= BIT(channel);
	else
		val &= ~BIT(channel);

	writel(val, de->mixer_regs + DE_MUX_BLEND_EN);
	de->channels[channel].enabled = enable;
}

static void sun60i_de_channel_set_format(struct sun60i_de *de, int channel)
{
	struct sun60i_de_channel *ch = &de->channels[channel];
	u32 val;

	val = readl(ch->regs + DE_CH_FORMAT);
	val &= ~0xff;
	val |= ch->format;
	writel(val, ch->regs + DE_CH_FORMAT);
}

static void sun60i_de_channel_set_address(struct sun60i_de *de, int channel,
					  dma_addr_t addr)
{
	writel(lower_32_bits(addr), de->channels[channel].regs + DE_CH_TOP_LADDR);
}

static void sun60i_de_channel_set_dimensions(struct sun60i_de *de, int channel,
					     u32 width, u32 height)
{
	u32 val;

	val = (height << 16) | width;
	writel(val, de->channels[channel].regs + DE_CH_SIZE);
}

static void sun60i_de_channel_set_pitch(struct sun60i_de *de, int channel,
					u32 pitch)
{
	writel(pitch, de->channels[channel].regs + DE_CH_PITCH);
}

static void sun60i_de_channel_set_coord(struct sun60i_de *de, int channel,
					u32 x, u32 y)
{
	u32 val;

	val = (y << 16) | x;
	writel(val, de->channels[channel].regs + DE_CH_COORD);
}

static void sun60i_de_set_blend_mode(struct sun60i_de *de, int channel,
				     u32 mode)
{
	writel(mode, de->channels[channel].regs + DE_CH_COLOR_CTL);
}

static void sun60i_de_set_background(struct sun60i_de *de, u32 color)
{
	writel(color, de->mixer_regs + DE_MUX_BG_COLOR);
}

static void sun60i_de_set_output_size(struct sun60i_de *de, u32 width,
				      u32 height)
{
	u32 val;

	val = (height << 16) | width;
	writel(val, de->mixer_regs + DE_MUX_BLEND_SIZE);
}

static void sun60i_de_alpha_blend_enable(struct sun60i_de *de, int channel,
					 bool premultiplied)
{
	u32 mode;

	mode = premultiplied ? DE_BLEND_MODE_PREMULTI : DE_BLEND_MODE_ALPHA;
	sun60i_de_set_blend_mode(de, channel, mode);
}

/*
 * DRM plane helpers
 */

static const u32 sun60i_de_formats[] = {
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_RGB565,
	DRM_FORMAT_NV12,
	DRM_FORMAT_NV21,
	DRM_FORMAT_YUV420,
};

static const struct drm_plane_funcs sun60i_de_plane_funcs = {
	.update_plane	= drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.destroy	= drm_plane_cleanup,
	duplicate_state = drm_atomic_helper_duplicate_plane_state,
	destroy_state	= drm_atomic_helper_destroy_plane_state,
};

static void sun60i_de_plane_setup(struct sun60i_de *de, int index,
				  struct drm_plane_state *state)
{
	struct drm_framebuffer *fb = state->fb;
	struct drm_gem_dma_obj *dma_obj;
	u32 fmt_idx;

	fmt_idx = sun60i_de_format_to_index(fb->format->format);
	if (fmt_idx < 0)
		return;

	de->channels[index].format = fmt_idx;

	sun60i_de_channel_set_format(de, index);
	sun60i_de_channel_set_dimensions(de, index, fb->width, fb->height);
	sun60i_de_channel_set_pitch(de, index, fb->pitches[0]);

	dma_obj = drm_gem_dma_object_get_from_gemm(&fb->obj[0]->addr, fb->obj[0]);
	sun60i_de_channel_set_address(de, index, dma_obj->dma_addr);

	sun60i_de_channel_enable(de, index, true);

	if (index == SUN60I_DE_PRIMARY_CHANNEL)
		sun60i_de_alpha_blend_enable(de, index, true);
}

static int sun60i_de_plane_atomic_check(struct drm_plane *plane,
					struct drm_atomic_state *state)
{
	struct drm_plane_state *new_state = drm_atomic_get_new_plane_state(state,
									   plane);

	if (!new_state->fb)
		return 0;

	if (new_state->src_w > SUN60I_DE_WIDTH_MAX ||
	    new_state->src_h > SUN60I_DE_HEIGHT_MAX)
		return -EINVAL;

	return 0;
}

static void sun60i_de_plane_atomic_update(struct drm_plane *plane,
					  struct drm_atomic_state *old_state)
{
	struct sun60i_de *de = to_sun60i_de(plane->dev);
	int index = plane->index;

	if (!plane->state->fb)
		return;

	sun60i_de_plane_setup(de, index, plane->state);
}

static const struct drm_plane_helper_funcs sun60i_de_plane_helper_funcs = {
	.atomic_check	= sun60i_de_plane_atomic_check,
	.atomic_update	= sun60i_de_plane_atomic_update,
	.atomic_disable	= sun60i_de_plane_atomic_update,
};

/*
 * CRTC helpers
 */

static const struct drm_crtc_funcs sun60i_de_crtc_funcs = {
	.reset		= drm_atomic_helper_crtc_reset,
	.destroy	= drm_crtc_cleanup,
	.set_config	= drm_atomic_helper_set_config,
	.page_flip	= drm_atomic_helper_page_flip,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_crtc_destroy_state,
};

static void sun60i_de_crtc_enable(struct drm_crtc *crtc,
				  struct drm_atomic_state *state)
{
	struct sun60i_de *de = to_sun60i_de(crtc->dev);

	clk_prepare_enable(de->dclk);
	clk_set_rate(de->dclk, crtc->state->adjusted_mode.clock * 1000);

	sun60i_de_set_background(de, 0x00000000);
	sun60i_de_set_output_size(de, crtc->state->adjusted_mode.hdisplay,
				  crtc->state->adjusted_mode.vdisplay);
}

static void sun60i_de_crtc_disable(struct drm_crtc *crtc,
				   struct drm_atomic_state *state)
{
	struct sun60i_de *de = to_sun60i_de(crtc->dev);
	int i;

	for (i = 0; i < SUN60I_DE_MAX_CHANNELS; i++)
		sun60i_de_channel_enable(de, i, false);

	clk_disable_unprepare(de->dclk);
}

static int sun60i_de_crtc_atomic_check(struct drm_crtc *crtc,
				       struct drm_atomic_state *state)
{
	return 0;
}

static void sun60i_de_crtc_atomic_flush(struct drm_crtc *crtc,
					struct drm_atomic_state *state)
{
	struct sun60i_de *de = to_sun60i_de(crtc->dev);
	u32 route = 0;
	int i;

	for (i = 0; i < SUN60I_DE_MAX_CHANNELS; i++) {
		if (de->channels[i].enabled)
			route |= BIT(i);
	}

	writel(route, de->mixer_regs + DE_MUX_BLEND_ROUTE);
}

static const struct drm_crtc_helper_funcs sun60i_de_crtc_helper_funcs = {
	.enable		= sun60i_de_crtc_enable,
	.disable	= sun60i_de_crtc_disable,
	.atomic_check	= sun60i_de_crtc_atomic_check,
	.atomic_flush	= sun60i_de_crtc_atomic_flush,
};

/*
 * IRQ handling
 */

static irqreturn_t sun60i_de_isr(int irq, void *data)
{
	struct sun60i_de *de = data;
	u32 status;

	status = readl(de->regs + DE_MUX_DEBUG_CFG);

	if (!status)
		return IRQ_NONE;

	writel(status, de->regs + DE_MUX_DEBUG_CFG);

	return IRQ_HANDLED;
}

/*
 * Clock and reset management
 */

static int sun60i_de_clks_init(struct sun60i_de *de)
{
	int ret;

	de->bus_clk = devm_clk_get(de->dev, "bus");
	if (IS_ERR(de->bus_clk))
		return dev_err_probe(de->dev, PTR_ERR(de->bus_clk),
				     "failed to get bus clock\n");

	de->mod_clk = devm_clk_get(de->dev, "mod");
	if (IS_ERR(de->mod_clk))
		return dev_err_probe(de->dev, PTR_ERR(de->mod_clk),
				     "failed to get mod clock\n");

	de->dclk = devm_clk_get(de->dev, "dclk");
	if (IS_ERR(de->dclk))
		return dev_err_probe(de->dev, PTR_ERR(de->dclk),
				     "failed to get display clock\n");

	ret = clk_set_rate(de->mod_clk, 432000000);
	if (ret)
		return dev_err_probe(de->dev, ret,
				     "failed to set module clock rate\n");

	return 0;
}

static int sun60i_de_hw_init(struct sun60i_de *de)
{
	int ret;

	ret = reset_control_deassert(de->rstc);
	if (ret) {
		dev_err(de->dev, "failed to deassert reset: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(de->bus_clk);
	if (ret) {
		dev_err(de->dev, "failed to enable bus clock: %d\n", ret);
		goto err_rst_assert;
	}

	ret = clk_prepare_enable(de->mod_clk);
	if (ret) {
		dev_err(de->dev, "failed to enable mod clock: %d\n", ret);
		goto err_bus_clk;
	}

	return 0;

err_bus_clk:
	clk_disable_unprepare(de->bus_clk);
err_rst_assert:
	reset_control_assert(de->rstc);

	return ret;
}

/*
 * Component binding
 */

static int sun60i_de_bind(struct device *dev, struct device *master,
			  void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sun60i_de *de;
	struct drm_device *drm;
	int i, ret, irq;

	de = devm_kzalloc(dev, sizeof(*de), GFP_KERNEL);
	if (!de)
		return -ENOMEM;

	spin_lock_init(&de->lock);
	dev_set_drvdata(dev, de);
	de->dev = dev;

	ret = sun60i_de_clks_init(de);
	if (ret)
		return ret;

	de->rstc = devm_reset_control_get_shared(dev, NULL);
	if (IS_ERR(de->rstc))
		return dev_err_probe(dev, PTR_ERR(de->rstc),
				     "failed to get reset control\n");

	ret = sun60i_de_hw_init(de);
	if (ret)
		return ret;

	de->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(de->regs)) {
		ret = PTR_ERR(de->regs);
		goto err_clk_disable;
	}

	de->mixer_regs = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(de->mixer_regs)) {
		ret = PTR_ERR(de->mixer_regs);
		goto err_clk_disable;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto err_clk_disable;
	}

	ret = devm_request_irq(dev, irq, sun60i_de_isr, 0, dev_name(dev), de);
	if (ret) {
		dev_err(dev, "failed to request IRQ %d: %d\n", irq, ret);
		goto err_clk_disable;
	}

	drm = drm_dev_alloc(&sun60i_de_driver, dev);
	if (IS_ERR(drm)) {
		ret = PTR_ERR(drm);
		goto err_clk_disable;
	}

	drm->mode_config.min_width = 16;
	drm->mode_config.min_height = 16;
	drm->mode_config.max_width = SUN60I_DE_WIDTH_MAX;
	drm->mode_config.max_height = SUN60I_DE_HEIGHT_MAX;
	drm->mode_config.funcs = &sun60i_de_mode_funcs;
	drm->mode_config.helper_private = &sun60i_de_mode_helper_funcs;

	de->drm = drm;

	/* Create overlay planes */
	for (i = 0; i < SUN60I_DE_MAX_CHANNELS; i++) {
		de->planes[i] = drm_universal_plane_init(drm, NULL,
				 BIT(i),
				 &sun60i_de_plane_funcs,
				 sun60i_de_formats,
				 ARRAY_SIZE(sun60i_de_formats),
				 NULL, DRM_PLANE_TYPE_OVERLAY, NULL);
		if (IS_ERR(de->planes[i])) {
			ret = PTR_ERR(de->planes[i]);
			goto err_drm_dev;
		}

		drm_plane_helper_add(de->planes[i],
				     &sun60i_de_plane_helper_funcs);
	}

	/* Set primary plane type */
	de->planes[SUN60I_DE_PRIMARY_CHANNEL]->type = DRM_PLANE_TYPE_PRIMARY;

	ret = drm_vblank_init(drm, 1);
	if (ret)
		goto err_drm_dev;

	ret = drmm_mode_config_init(drm);
	if (ret)
		goto err_drm_dev;

	ret = drm_dev_register(drm, 0);
	if (ret)
		goto err_drm_dev;

	return 0;

err_drm_dev:
	drm_dev_put(drm);
err_clk_disable:
	clk_disable_unprepare(de->mod_clk);
	clk_disable_unprepare(de->bus_clk);
	reset_control_assert(de->rstc);

	return ret;
}

static void sun60i_de_unbind(struct device *dev, struct device *master,
			     void *data)
{
	struct sun60i_de *de = dev_get_drvdata(dev);
	struct drm_device *drm = de->drm;

	drm_dev_unplug(drm);
	drm_atomic_helper_shutdown(drm);

	clk_disable_unprepare(de->mod_clk);
	clk_disable_unprepare(de->bus_clk);
	reset_control_assert(de->rstc);
}

static const struct component_ops sun60i_de_ops = {
	.bind	= sun60i_de_bind,
	.unbind	= sun60i_de_unbind,
};

static int sun60i_de_probe(struct platform_device *pdev)
{
	return component_add(&pdev->dev, &sun60i_de_ops);
}

static void sun60i_de_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &sun60i_de_ops);
}

static const struct of_device_id sun60i_de_match[] = {
	{ .compatible = "allwinner,sun60i-a733-display-engine" },
	{ }
};
MODULE_DEVICE_TABLE(of, sun60i_de_match);

struct platform_driver sun60i_de_platform_driver = {
	.probe	= sun60i_de_probe,
	.remove	= sun60i_de_remove,
	.driver	= {
		.name		= "sun60i-de",
		.of_match_table	= sun60i_de_match,
	},
};
module_platform_driver(sun60i_de_platform_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Allwinner Technology Co., Ltd.");
MODULE_DESCRIPTION("Allwinner A733 Display Engine 2 driver");
