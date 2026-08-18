// SPDX-License-Identifier: GPL-2.0-only
/*
 * Allwinner A733 (sun60iw2) DRM master driver
 *
 * Ties together the Display Engine (DE), TCON, and HDMI components.
 *
 * Copyright (C) 2026 Allwinner Technology Co., Ltd.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include <drm/drm_device.h>
#include <drm/drm_fbdev_generic.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem.h>
#include <drm/drm_gem_dma_helper.h>
#include <drm/drm_probe_helper.h>

struct sun60i_drm_private {
	const struct drm_mode_config_funcs mode_config_funcs;
};

/*
 * Mode config helpers
 */

static struct drm_framebuffer *
sun60i_drm_fb_create(struct drm_device *drm, struct drm_file *file_priv,
		     const struct drm_mode_fb_cmd2 *mode_cmd)
{
	return drm_gem_fb_create(drm, file_priv, mode_cmd);
}

static const struct drm_mode_config_funcs sun60i_drm_mode_config_funcs = {
	.create_blob	= NULL,
	.atomic_check	= drm_atomic_helper_check,
	.atomic_commit	= drm_atomic_helper_commit,
	.fb_create	= sun60i_drm_fb_create,
};

/*
 * Platform driver registration
 */

extern struct platform_driver sun60i_de_platform_driver;
extern struct platform_driver sun60i_tcon_platform_driver;
extern struct platform_driver sun60i_hdmi_platform_driver;

static int sun60i_drm_register_components(void)
{
	int ret;

	ret = platform_driver_register(&sun60i_de_platform_driver);
	if (ret)
		return ret;

	ret = platform_driver_register(&sun60i_tcon_platform_driver);
	if (ret)
		goto err_de;

	ret = platform_driver_register(&sun60i_hdmi_platform_driver);
	if (ret)
		goto err_tcon;

	return 0;

err_tcon:
	platform_driver_unregister(&sun60i_tcon_platform_driver);
err_de:
	platform_driver_unregister(&sun60i_de_platform_driver);

	return ret;
}

static void sun60i_drm_unregister_components(void)
{
	platform_driver_unregister(&sun60i_hdmi_platform_driver);
	platform_driver_unregister(&sun60i_tcon_platform_driver);
	platform_driver_unregister(&sun60i_de_platform_driver);
}

static int sun60i_drm_pdev_compare(struct device *dev, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct platform_device *match = data;

	return pdev == match;
}

static int sun60i_drm_bind(struct device *dev)
{
	struct drm_device *drm;
	int ret;

	drm = drm_dev_alloc(&sun60i_drm_driver, dev);
	if (IS_ERR(drm))
		return PTR_ERR(drm);

	drm->mode_config.funcs = &sun60i_drm_mode_config_funcs;
	drm->mode_config.min_width = 16;
	drm->mode_config.min_height = 16;
	drm->mode_config.max_width = 4096;
	drm->mode_config.max_height = 4096;
	drm->mode_config.preferred_depth = 32;

	ret = drmm_mode_config_init(drm);
	if (ret) {
		drm_dev_put(drm);
		return ret;
	}

	ret = drm_dev_register(drm, 0);
	if (ret) {
		drm_dev_put(drm);
		return ret;
	}

	drm_fbdev_generic_setup(drm, 32);

	dev_set_drvdata(dev, drm);

	return 0;
}

static void sun60i_drm_unbind(struct device *dev)
{
	struct drm_device *drm = dev_get_drvdata(dev);

	if (!drm)
		return;

	drm_dev_unplug(drm);
	drm_atomic_helper_shutdown(drm);
	drm_dev_put(drm);

	dev_set_drvdata(dev, NULL);
}

static const struct drm_master_funcs sun60i_drm_master_funcs = {
	.release = NULL,
};

static int sun60i_drm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	return component_bind_all(dev, NULL);
}

static void sun60i_drm_remove(struct platform_device *pdev)
{
	component_unbind_all(&pdev->dev, NULL);
}

static const struct of_device_id sun60i_drm_match[] = {
	{ .compatible = "allwinner,sun60i-a733-display-subsystem" },
	{ }
};
MODULE_DEVICE_TABLE(of, sun60i_drm_match);

struct platform_driver sun60i_drm_platform_driver = {
	.probe	= sun60i_drm_probe,
	.remove	= sun60i_drm_remove,
	.driver	= {
		.name		= "sun60i-drm",
		.of_match_table	= sun60i_drm_match,
	},
};

static int __init sun60i_drm_init(void)
{
	int ret;

	ret = sun60i_drm_register_components();
	if (ret)
		return ret;

	ret = platform_driver_register(&sun60i_drm_platform_driver);
	if (ret)
		goto err_unregister;

	return 0;

err_unregister:
	sun60i_drm_unregister_components();
	return ret;
}

static void __exit sun60i_drm_exit(void)
{
	platform_driver_unregister(&sun60i_drm_platform_driver);
	sun60i_drm_unregister_components();
}

module_init(sun60i_drm_init);
module_exit(sun60i_drm_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Allwinner Technology Co., Ltd.");
MODULE_DESCRIPTION("Allwinner A733 DRM display subsystem driver");
