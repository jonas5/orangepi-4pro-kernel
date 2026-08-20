// SPDX-License-Identifier: GPL-2.0-only
/*
 * Allwinner A733 (sun60iw2) Audio Machine driver
 *
 * Audio machine driver connecting I2S0 controller and ES8388 codec.
 *
 * Copyright (C) 2026 Allwinner Technology Co., Ltd.
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <sound/asoundef.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/jack.h>

#define DRV_NAME "sun60i-audio"

struct sun60i_audio_card {
	struct snd_soc_card		card;
	struct snd_soc_dai_link		dai_link;
	struct snd_soc_dai_link_component	dlc[3];
	struct snd_jack			*jack;
};

static const struct snd_soc_dapm_widget sun60i_audio_widgets[] = {
	SND_SOC_DAPM_HP("Headphone", NULL),
	SND_SOC_DAPM_SPK("Speaker", NULL),
	SND_SOC_DAPM_MIC("Internal Mic", NULL),
	SND_SOC_DAPM_MIC("External Mic", NULL),
};

static const struct snd_soc_dapm_route sun60i_audio_routes[] = {
	{ "Headphone", NULL, "HPOL" },
	{ "Headphone", NULL, "HPOR" },
	{ "Speaker",  NULL, "SPKOUT" },
	{ "MIC1", NULL, "Internal Mic" },
	{ "MIC2", NULL, "External Mic" },
};

static int sun60i_audio_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static int sun60i_audio_hw_params(struct snd_pcm_substream *substream,
				  struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component = snd_soc_rtdcom_lookup(rtd,
						   "es8388-codec");

	snd_soc_component_set_sysclk(component, 0, 0,
				     params_rate(params) * 256,
				     SND_SOC_CLOCK_OUT);

	return 0;
}

static const struct snd_soc_ops sun60i_audio_ops = {
	.startup	= sun60i_audio_startup,
	.hw_params	= sun60i_audio_hw_params,
};

static int sun60i_audio_init(struct snd_soc_pcm_runtime *rtd)
{
	return 0;
}

static int sun60i_audio_of_xlate_dai_name(struct snd_soc_component *component,
					  const struct of_phandle_args *args,
					  const char **dai_name)
{
	*dai_name = "ES8388 HiFi";
	return 0;
}

static const struct snd_soc_component_driver sun60i_audio_component = {
	.of_xlate_dai_name	= sun60i_audio_of_xlate_dai_name,
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
};

static int sun60i_audio_probe(struct platform_device *pdev)
{
	struct sun60i_audio_card *priv;
	struct device_node *i2s_node, *codec_node;
	struct snd_soc_dai_link *dai;
	struct snd_soc_dai_link_component *dlc;
	int ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	dlc = priv->dlc;

	/* Find I2S DAI */
	i2s_node = of_parse_phandle(pdev->dev.of_node,
				    "allwinner,i2s-controller", 0);
	if (!i2s_node) {
		dev_err(&pdev->dev, "failed to get I2S controller\n");
		return -ENODEV;
	}

	/* Find codec */
	codec_node = of_parse_phandle(pdev->dev.of_node,
				      "allwinner,codec", 0);
	if (!codec_node) {
		dev_err(&pdev->dev, "failed to get codec\n");
		of_node_put(i2s_node);
		return -ENODEV;
	}

	dai = &priv->dai_link;
	dai->name		= "sun60i-audio";
	dai->stream_name	= "sun60i-audio";
	dai->cpus		= &dlc[0];
	dai->cpus->dai_name	= "ES8388 HiFi";
	dai->codecs		= &dlc[1];
	dai->codecs->dai_name	= "ES8388 HiFi";
	dai->codecs->of_node	= codec_node;
	dai->platforms		= &dlc[2];
	dai->platforms->of_node	= i2s_node;
	dai->num_cpus		= 1;
	dai->num_codecs		= 1;
	dai->num_platforms	= 1;
	dai->init		= sun60i_audio_init;
	dai->ops			= &sun60i_audio_ops;
	dai->dai_fmt		= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
				  SND_SOC_DAIFMT_CBP_CFC;

	priv->card.dai_link	= dai;
	priv->card.num_links	= 1;
	priv->card.owner	= THIS_MODULE;
	priv->card.dev		= &pdev->dev;
	priv->card.name		= "Sun60i A733 Audio";
	priv->card.dapm_widgets	= sun60i_audio_widgets;
	priv->card.num_dapm_widgets = ARRAY_SIZE(sun60i_audio_widgets);
	priv->card.dapm_routes	= sun60i_audio_routes;
	priv->card.num_dapm_routes = ARRAY_SIZE(sun60i_audio_routes);

	ret = devm_snd_soc_register_card(&pdev->dev, &priv->card);
	if (ret) {
		dev_err(&pdev->dev, "failed to register card: %d\n", ret);
		goto err_node_put;
	}

	platform_set_drvdata(pdev, priv);

err_node_put:
	of_node_put(i2s_node);
	of_node_put(codec_node);

	return ret;
}

static void sun60i_audio_remove(struct platform_device *pdev)
{
}

static const struct of_device_id sun60i_audio_match[] = {
	{ .compatible = "allwinner,sun60i-a733-audio" },
	{ }
};
MODULE_DEVICE_TABLE(of, sun60i_audio_match);

struct platform_driver sun60i_audio_platform_driver = {
	.probe	= sun60i_audio_probe,
	.remove	= sun60i_audio_remove,
	.driver	= {
		.name		= DRV_NAME,
		.of_match_table	= sun60i_audio_match,
	},
};
module_platform_driver(sun60i_audio_platform_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Allwinner Technology Co., Ltd.");
MODULE_DESCRIPTION("Allwinner A733 Audio Machine driver");
