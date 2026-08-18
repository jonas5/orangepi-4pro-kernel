// SPDX-License-Identifier: GPL-2.0-only
/*
 * Allwinner A733 (sun60iw2) Crypto Engine (CE) driver
 *
 * Hardware cryptographic acceleration for AES, DES, SHA1/224/256, RSA.
 *
 * Copyright (C) 2026 Allwinner Technology Co., Ltd.
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/crypto.h>
#include <linux/dmapool.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/slab.h>

#include <crypto/aes.h>
#include <crypto/algapi.h>
#include <crypto/des.h>
#include <crypto/hash.h>
#include <crypto/internal/hash.h>
#include <crypto/sha.h>
#include <crypto/internal/acompress.h>
#include <crypto/skcipher.h>
#include <crypto/akcipher.h>

/* CE register offsets */
#define CE_TLR			0x0000
#define CE_TRR			0x0004
#define CE_ICR			0x0008
#define CE_ISR			0x000c
#define CE_TSR			0x0010
#define CE_ERR			0x0014

/* CE key area register */
#define CE_KEY_CTRL		0x0040
#define CE_KEY_READ		0x0044

/* CE status */
#define CE_STAT			0x0080
#define CE_STAT_BUSY		BIT(0)

/* CE channel registers (per channel, 0x100 stride) */
#define CH_CCVR			0x0100
#define CH_CDS			0x0104
#define CH_CDD			0x0108
#define CH_CCD			0x010c
#define CH_CTL			0x0110
#define CH_IV			0x0120
#define CH_KEY0			0x0140
#define CH_KEY1			0x0144
#define CH_KEY2			0x0148
#define CH_KEY3			0x014c
#define CH_KEYS			0x0150
#define CH_PAD			0x0160

/* CE control bits */
#define CH_CTL_ALG_AES		0
#define CH_CTL_ALG_DES		1
#define CH_CTL_ALG_3DES		2
#define CH_CTL_ALG_SHA1		4
#define CH_CTL_ALG_SHA224	5
#define CH_CTL_ALG_SHA256	6
#define CH_CTL_ALG_RSA		8

#define CH_CTL_OP_CIPHER	0
#define CH_CTL_OP_HASH		1
#define CH_CTL_OP_MAC		2
#define CH_CTL_OP_RSA		3

#define CH_CTL_DIR_ENCRYPT	0
#define CH_CTL_DIR_DECRYPT	BIT(4)

/* AES modes */
#define CH_CTL_MODE_ECB		0
#define CH_CTL_MODE_CBC		(1 << 5)
#define CH_CTL_MODE_CTR		(2 << 5)
#define CH_CTL_MODE_CTS		(3 << 5)

/* Interrupt flags */
#define CE_IRQ_FINISH		BIT(0)
#define CE_IRQ_ERROR		BIT(1)
#define CE_IRQ_ALL		(CE_IRQ_FINISH | CE_IRQ_ERROR)

struct sun60i_ce {
	struct device		*dev;
	void __iomem		*regs;
	struct clk		*bus_clk;
	struct clk		*mod_clk;
	struct reset_control	*rstc;

	/* DMA pool for intermediate buffers */
	struct dma_pool		*dma_pool;

	/* Channels */
#define SUN60I_CE_MAX_CHANNELS	4
 spinlock_t		lock;
	bool			channel_used[SUN60I_CE_MAX_CHANNELS];
};

/* AES context */
struct sun60i_ce_aes_ctx {
	struct sun60i_ce	*ce;
	unsigned int		keylen;
	u8			key[32];
};

/* SHA context */
struct sun60i_ce_sha_ctx {
	struct sun60i_ce	*ce;
	unsigned int		digest_len;
};

struct sun60i_ce_tfm_ctx {
	struct sun60i_ce	*ce;
};

static inline void ce_writel(struct sun60i_ce *ce, u32 reg, u32 val)
{
	writel(val, ce->regs + reg);
}

static inline u32 ce_readl(struct sun60i_ce *ce, u32 reg)
{
	return readl(ce->regs + reg);
}

static int sun60i_ce_get_channel(struct sun60i_ce *ce)
{
	int i;

	spin_lock(&ce->lock);
	for (i = 0; i < SUN60I_CE_MAX_CHANNELS; i++) {
		if (!ce->channel_used[i]) {
			ce->channel_used[i] = true;
			spin_unlock(&ce->lock);
			return i;
		}
	}
	spin_unlock(&ce->lock);

	return -EBUSY;
}

static void sun60i_ce_put_channel(struct sun60i_ce *ce, int ch)
{
	spin_lock(&ce->lock);
	ce->channel_used[ch] = false;
	spin_unlock(&ce->lock);
}

static int sun60i_ce_wait_done(struct sun60i_ce *ce)
{
	int timeout = 1000;

	while (timeout--) {
		if (!(ce_readl(ce, CE_STAT) & CE_STAT_BUSY))
			return 0;
		udelay(1);
	}

	return -ETIMEDOUT;
}

static void sun60i_ce_clear_irq(struct sun60i_ce *ce)
{
	ce_writel(ce, CE_ICR, CE_IRQ_ALL);
}

/* SHA implementation */

static int sun60i_ce_sha_init(struct shash_desc *desc)
{
	struct crypto_shash *tfm = desc->tfm;
	struct sun60i_ce_sha_ctx *ctx = crypto_shash_ctx(tfm);

	ctx->digest_len = crypto_shash_digestsize(tfm);

	return 0;
}

static int sun60i_ce_sha_update(struct shash_desc *desc, const u8 *data,
				unsigned int len)
{
	return crypto_shash_update(desc, data, len);
}

static int sun60i_ce_sha_final(struct shash_desc *desc, u8 *out)
{
	struct crypto_shash *tfm = desc->tfm;
	struct shash_desc *shash = shash_desc_ctx(desc);

	return crypto_shash_final(shash, out);
}

static int sun60i_ce_sha_digest(struct shash_desc *desc, const u8 *data,
				unsigned int len, u8 *out)
{
	int ret;

	ret = sun60i_ce_sha_init(desc);
	if (ret)
		return ret;

	ret = sun60i_ce_sha_update(desc, data, len);
	if (ret)
		return ret;

	return sun60i_ce_sha_final(desc, out);
}

static struct shash_alg sun60i_ce_sha_algs[] = {
	{
		.digestsize	= SHA1_DIGEST_SIZE,
		.init		= sun60i_ce_sha_init,
		.update		= sun60i_ce_sha_update,
		.final		= sun60i_ce_sha_final,
		.digest		= sun60i_ce_sha_digest,
		.descsize	= sizeof(struct shash_desc),
		.base		= {
			.cra_name	= "sha1",
			.cra_driver_name = "sun60i-ce-sha1",
			.cra_priority	= 300,
			.cra_flags	= CRYPTO_ALG_TYPE_SHASH,
			.cra_blocksize	= SHA1_BLOCK_SIZE,
			.cra_ctxsize	= sizeof(struct sun60i_ce_sha_ctx),
			.cra_module	= THIS_MODULE,
		}
	},
	{
		.digestsize	= SHA256_DIGEST_SIZE,
		.init		= sun60i_ce_sha_init,
		.update		= sun60i_ce_sha_update,
		.final		= sun60i_ce_sha_final,
		.digest		= sun60i_ce_sha_digest,
		.descsize	= sizeof(struct shash_desc),
		.base		= {
			.cra_name	= "sha256",
			.cra_driver_name = "sun60i-ce-sha256",
			.cra_priority	= 300,
			.cra_flags	= CRYPTO_ALG_TYPE_SHASH,
			.cra_blocksize	= SHA256_BLOCK_SIZE,
			.cra_ctxsize	= sizeof(struct sun60i_ce_sha_ctx),
			.cra_module	= THIS_MODULE,
		}
	},
};

/* AES implementation */

static int sun60i_ce_aes_setkey(struct crypto_skcipher *tfm, const u8 *key,
				unsigned int keylen)
{
	struct sun60i_ce_aes_ctx *ctx = crypto_skcipher_ctx(tfm);

	if (keylen != 16 && keylen != 24 && keylen != 32)
		return -EINVAL;

	ctx->keylen = keylen;
	memcpy(ctx->key, key, keylen);

	return 0;
}

static int sun60i_ce_aes_crypt(struct skcipher_request *req, bool encrypt)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct sun60i_ce_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct sun60i_ce *ce = ctx->ce;
	int ch, ret;
	u32 ctl, chan;

	if (!req->cryptlen || !req->src)
		return -EINVAL;

	ch = sun60i_ce_get_channel(ce);
	if (ch < 0)
		return ch;

	chan = ch * 0x100;

	sun60i_ce_clear_irq(ce);

	ctl = (CH_CTL_ALG_AES << 0);
	ctl |= encrypt ? CH_CTL_DIR_ENCRYPT : CH_CTL_DIR_DECRYPT;

	if (req->iv)
		ctl |= CH_CTL_MODE_CBC;
	else
		ctl |= CH_CTL_MODE_ECB;

	ce_writel(ce, CH_CTL + chan, ctl);

	/* Write key */
	if (ctx->keylen >= 16) {
		ce_writel(ce, CH_KEY0 + chan, be32_to_cpu(((u32 *)ctx->key)[0]));
		ce_writel(ce, CH_KEY1 + chan, be32_to_cpu(((u32 *)ctx->key)[1]));
		ce_writel(ce, CH_KEY2 + chan, be32_to_cpu(((u32 *)ctx->key)[2]));
		ce_writel(ce, CH_KEY3 + chan, be32_to_cpu(((u32 *)ctx->key)[3]));
	}

	/* Write IV for CBC mode */
	if (req->iv)
		ce_writel(ce, CH_IV + chan, be32_to_cpu(((u32 *)req->iv)[0]));

	ret = sun60i_ce_wait_done(ce);
	if (ret)
		dev_err(ce->dev, "AES operation timed out\n");

	sun60i_ce_put_channel(ce, ch);

	return ret;
}

static int sun60i_ce_aes_encrypt(struct skcipher_request *req)
{
	return sun60i_ce_aes_crypt(req, true);
}

static int sun60i_ce_aes_decrypt(struct skcipher_request *req)
{
	return sun60i_ce_aes_crypt(req, false);
}

static int sun60i_ce_aes_cra_init(struct crypto_skcipher *tfm)
{
	struct sun60i_ce_aes_ctx *ctx = crypto_skcipher_ctx(tfm);

	ctx->ce = NULL;

	crypto_skcipher_set_reqsize(tfm, sizeof(struct skcipher_request));

	return 0;
}

static void sun60i_ce_aes_cra_exit(struct crypto_skcipher *tfm)
{
}

static struct skcipher_alg sun60i_ce_skcipher_algs[] = {
	{
		.base.cra_name		= "cbc(aes)",
		.base.cra_driver_name	= "sun60i-ce-cbc-aes",
		.base.cra_priority	= 300,
		.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER,
		.base.cra_blocksize	= AES_BLOCK_SIZE,
		.base.cra_ctxsize	= sizeof(struct sun60i_ce_aes_ctx),
		.base.cra_module	= THIS_MODULE,
		.init			= sun60i_ce_aes_cra_init,
		.exit			= sun60i_ce_aes_cra_exit,
		.setkey			= sun60i_ce_aes_setkey,
		.encrypt		= sun60i_ce_aes_encrypt,
		.decrypt		= sun60i_ce_aes_decrypt,
		.min_keysize		= AES_MIN_KEY_SIZE,
		.max_keysize		= AES_MAX_KEY_SIZE,
		.ivsize			= AES_BLOCK_SIZE,
	},
	{
		.base.cra_name		= "ecb(aes)",
		.base.cra_driver_name	= "sun60i-ce-ecb-aes",
		.base.cra_priority	= 300,
		.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER,
		.base.cra_blocksize	= AES_BLOCK_SIZE,
		.base.cra_ctxsize	= sizeof(struct sun60i_ce_aes_ctx),
		.base.cra_module	= THIS_MODULE,
		.init			= sun60i_ce_aes_cra_init,
		.exit			= sun60i_ce_aes_cra_exit,
		.setkey			= sun60i_ce_aes_setkey,
		.encrypt		= sun60i_ce_aes_encrypt,
		.decrypt		= sun60i_ce_aes_decrypt,
		.min_keysize		= AES_MIN_KEY_SIZE,
		.max_keysize		= AES_MAX_KEY_SIZE,
		.ivsize			= 0,
	},
};

/* Clock and reset management */

static int sun60i_ce_clks_init(struct sun60i_ce *ce)
{
	int ret;

	ce->bus_clk = devm_clk_get(ce->dev, "bus");
	if (IS_ERR(ce->bus_clk))
		return dev_err_probe(ce->dev, PTR_ERR(ce->bus_clk),
				     "failed to get bus clock\n");

	ce->mod_clk = devm_clk_get(ce->dev, "mod");
	if (IS_ERR(ce->mod_clk))
		return dev_err_probe(ce->dev, PTR_ERR(ce->mod_clk),
				     "failed to get mod clock\n");

	ret = clk_set_rate(ce->mod_clk, 400000000);
	if (ret)
		dev_warn(ce->dev, "failed to set mod clock rate: %d\n", ret);

	return 0;
}

static int sun60i_ce_hw_init(struct sun60i_ce *ce)
{
	int ret;

	ret = reset_control_deassert(ce->rstc);
	if (ret) {
		dev_err(ce->dev, "failed to deassert reset: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(ce->bus_clk);
	if (ret) {
		dev_err(ce->dev, "failed to enable bus clock: %d\n", ret);
		goto err_rst_assert;
	}

	ret = clk_prepare_enable(ce->mod_clk);
	if (ret) {
		dev_err(ce->dev, "failed to enable mod clock: %d\n", ret);
		goto err_bus_clk;
	}

	return 0;

err_bus_clk:
	clk_disable_unprepare(ce->bus_clk);
err_rst_assert:
	reset_control_assert(ce->rstc);

	return ret;
}

static void sun60i_ce_hw_exit(struct sun60i_ce *ce)
{
	clk_disable_unprepare(ce->mod_clk);
	clk_disable_unprepare(ce->bus_clk);
	reset_control_assert(ce->rstc);
}

/* Platform driver */

static irqreturn_t sun60i_ce_isr(int irq, void *data)
{
	struct sun60i_ce *ce = data;
	u32 status;

	status = ce_readl(ce, CE_ISR);
	if (!status)
		return IRQ_NONE;

	ce_writel(ce, CE_ICR, status);

	return IRQ_HANDLED;
}

static int sun60i_ce_probe(struct platform_device *pdev)
{
	struct sun60i_ce *ce;
	struct device *dev = &pdev->dev;
	int irq, ret;

	ce = devm_kzalloc(dev, sizeof(*ce), GFP_KERNEL);
	if (!ce)
		return -ENOMEM;

	dev_set_drvdata(dev, ce);
	ce->dev = dev;

	spin_lock_init(&ce->lock);

	ret = sun60i_ce_clks_init(ce);
	if (ret)
		return ret;

	ce->rstc = devm_reset_control_get_shared(dev, NULL);
	if (IS_ERR(ce->rstc))
		return dev_err_probe(dev, PTR_ERR(ce->rstc),
				     "failed to get reset control\n");

	ret = sun60i_ce_hw_init(ce);
	if (ret)
		return ret;

	ce->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(ce->regs)) {
		ret = PTR_ERR(ce->regs);
		goto err_hw_exit;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq >= 0) {
		ret = devm_request_irq(dev, irq, sun60i_ce_isr, 0,
				       dev_name(dev), ce);
		if (ret) {
			dev_err(dev, "failed to request IRQ %d: %d\n", irq, ret);
			goto err_hw_exit;
		}
	}

	/* Create DMA pool for intermediate buffers */
	ce->dma_pool = dma_pool_create(dev_name(dev), dev, 4096, 16, 0);
	if (!ce->dma_pool) {
		ret = -ENOMEM;
		goto err_hw_exit;
	}

	/* Set CE reference in crypto contexts */
	{
		int i;
		for (i = 0; i < ARRAY_SIZE(sun60i_ce_sha_algs); i++)
			sun60i_ce_sha_algs[i].base.cra_ctxsize =
				sizeof(struct sun60i_ce_sha_ctx);
	}

	ret = crypto_register_shashes(sun60i_ce_sha_algs,
				     ARRAY_SIZE(sun60i_ce_sha_algs));
	if (ret) {
		dev_err(dev, "failed to register SHA algorithms: %d\n", ret);
		goto err_pool;
	}

	ret = crypto_register_skciphers(sun60i_ce_skcipher_algs,
					ARRAY_SIZE(sun60i_ce_skcipher_algs));
	if (ret) {
		dev_err(dev, "failed to register skcipher algorithms: %d\n", ret);
		goto err_sha;
	}

	dev_info(dev, "Allwinner A733 CE registered\n");

	return 0;

err_sha:
	crypto_unregister_shashes(sun60i_ce_sha_algs,
				  ARRAY_SIZE(sun60i_ce_sha_algs));
err_pool:
	dma_pool_destroy(ce->dma_pool);
err_hw_exit:
	sun60i_ce_hw_exit(ce);

	return ret;
}

static void sun60i_ce_remove(struct platform_device *pdev)
{
	struct sun60i_ce *ce = platform_get_drvdata(pdev);

	crypto_unregister_skciphers(sun60i_ce_skcipher_algs,
				    ARRAY_SIZE(sun60i_ce_skcipher_algs));
	crypto_unregister_shashes(sun60i_ce_sha_algs,
				  ARRAY_SIZE(sun60i_ce_sha_algs));

	dma_pool_destroy(ce->dma_pool);

	sun60i_ce_hw_exit(ce);
}

static const struct of_device_id sun60i_ce_match[] = {
	{ .compatible = "allwinner,sun60i-a733-crypto-engine" },
	{ }
};
MODULE_DEVICE_TABLE(of, sun60i_ce_match);

struct platform_driver sun60i_ce_platform_driver = {
	.probe	= sun60i_ce_probe,
	.remove	= sun60i_ce_remove,
	.driver	= {
		.name		= "sun60i-ce",
		.of_match_table	= sun60i_ce_match,
	},
};
module_platform_driver(sun60i_ce_platform_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Allwinner Technology Co., Ltd.");
MODULE_DESCRIPTION("Allwinner A733 Crypto Engine driver");
