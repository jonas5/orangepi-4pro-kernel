// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * MMC host driver for Allwinner sun60i-a733 SoC.
 *
 * Based on the sunxi-mmc driver and adapted for the A733 variant
 * with enhanced DMA, UHS timing support (SDR50/SDR104), and HS400.
 *
 * Copyright (C) 2024 Allwinner Technology Co., Ltd.
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>

#include "sun20i-d1-mmc.h"

#define DRV_NAME	"sun20i-d1-mmc"

/* Poll interval for card detect */
#define SUNXI_CARD_DETECT_INTERVAL_MS	200

/* DMA transfer alignment */
#define SUNXI_DMA_ALIGN		512
#define SUNXI_DMA_MAX_LEN	(SZ_512K)

static int sunxi_mmc_init_host(struct sunxi_mmc_host *host);
static void sunxi_mmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios);
static int sunxi_mmc_get_ro(struct mmc_host *mmc);
static int sunxi_mmc_get_cd(struct mmc_host *mmc);
static void sunxi_mmc_enable_card_detection(struct mmc_host *mmc);

static inline u32 sunxi_mmc_readl(struct sunxi_mmc_host *host, u32 reg)
{
	return readl(host->reg_base + reg);
}

static inline void sunxi_mmc_writel(struct sunxi_mmc_host *host,
				    u32 reg, u32 val)
{
	writel(val, host->reg_base + reg);
}

static inline void sunxi_mmc_set_bit(struct sunxi_mmc_host *host,
				     u32 reg, u32 bit)
{
	u32 val = sunxi_mmc_readl(host, reg);

	val |= bit;
	sunxi_mmc_writel(host, reg, val);
}

static inline void sunxi_mmc_clr_bit(struct sunxi_mmc_host *host,
				     u32 reg, u32 bit)
{
	u32 val = sunxi_mmc_readl(host, reg);

	val &= ~bit;
	sunxi_mmc_writel(host, reg, val);
}

/* Reset the controller */
static int sunxi_mmc_reset(struct sunxi_mmc_host *host, u32 reset_val)
{
	unsigned long timeout;

	sunxi_mmc_writel(host, SDC_CTRL,
			 SDC_CTRL_EN | reset_val);

	timeout = jiffies + msecs_to_jiffies(500);
	while (time_is_before_jiffies(timeout)) {
		if (!(sunxi_mmc_readl(host, SDC_CTRL) & reset_val))
			return 0;
	}

	dev_err(host->dev, "Controller reset timeout\n");
	return -ETIMEDOUT;
}

/* Enable clocks and release reset */
static int sunxi_mmc_clks_enable(struct sunxi_mmc_host *host)
{
	int ret;

	ret = clk_prepare_enable(host->clk_bus);
	if (ret) {
		dev_err(host->dev, "Failed to enable bus clock: %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(host->clk_mod);
	if (ret) {
		dev_err(host->dev, "Failed to enable module clock: %d\n", ret);
		goto err_disable_bus;
	}

	ret = clk_prepare_enable(host->clk_mmc);
	if (ret) {
		dev_err(host->dev, "Failed to enable MMC clock: %d\n", ret);
		goto err_disable_mod;
	}

	if (host->reset) {
		ret = reset_control_deassert(host->reset);
		if (ret) {
			dev_err(host->dev, "Failed to release reset: %d\n",
				ret);
			goto err_disable_mmc;
		}
	}

	return 0;

err_disable_mmc:
	clk_disable_unprepare(host->clk_mmc);
err_disable_mod:
	clk_disable_unprepare(host->clk_mod);
err_disable_bus:
	clk_disable_unprepare(host->clk_bus);
	return ret;
}

static void sunxi_mmc_clks_disable(struct sunxi_mmc_host *host)
{
	if (host->reset)
		reset_control_assert(host->reset);

	clk_disable_unprepare(host->clk_mmc);
	clk_disable_unprepare(host->clk_mod);
	clk_disable_unprepare(host->clk_bus);
}

/* Set the clock divider and source for the MMC controller */
static int sunxi_mmc_set_clk(struct sunxi_mmc_host *host,
			     unsigned int hz)
{
	unsigned int clk, div;
	u32 reg;

	if (hz > 52000000) {
		host->using_52m_clk = true;
		clk = clk_get_rate(host->clk_mod);
	} else {
		host->using_52m_clk = false;
		clk = clk_get_rate(host->clk_mmc);
	}

	if (hz <= 400000) {
		div = 0xf;  /* Div = 1/16 for 400K */
		host->fclk_div = 0xf;
	} else if (hz <= 25000000) {
		div = 0xf;
		host->fclk_div = 0xf;
	} else if (hz <= 50000000) {
		div = 0x5;
		host->fclk_div = 0x5;
	} else {
		div = 0;
		host->fclk_div = 0;
	}

	/* Set clock divider */
	reg = sunxi_mmc_readl(host, SDC_CLKDIV);
	reg &= ~(SDC_CLKDIV_DIV1_MASK | SDC_CLKDIV_DIV2_MASK);
	reg |= div;
	sunxi_mmc_writel(host, SDC_CLKDIV, reg);

	/* Select clock source */
	reg = sunxi_mmc_readl(host, SDC_CKGEN);
	reg &= ~SDC_CKGEN_OUT_MASK;
	if (host->using_52m_clk)
		reg |= SDC_CKGEN_OUT_TEST;
	sunxi_mmc_writel(host, SDC_CKGEN, reg);

	/* Ensure clock is enabled */
	reg = sunxi_mmc_readl(host, SDC_CTRL);
	reg |= SDC_CTRL_EN;
	sunxi_mmc_writel(host, SDC_CTRL, reg);

	return 0;
}

/* Calculate the sample point delay based on timing */
static void sunxi_mmc_set_sample_delay(struct sunxi_mmc_host *host,
				       unsigned int timing)
{
	u32 reg;

	reg = sunxi_mmc_readl(host, SDC_SMPL_DL_CT);
	reg &= ~SDC_CKGEN_SMPL_DLY_MASK;

	switch (timing) {
	case MMC_TIMING_UHS_SDR50:
	case MMC_TIMING_MMC_HS200:
		reg |= SDC_CKGEN_SMPL_DLY_4;
		break;
	case MMC_TIMING_UHS_SDR104:
	case MMC_TIMING_MMC_HS400:
	case MMC_TIMING_MMC_HS400_ES:
		reg |= SDC_CKGEN_SMPL_DLY_2;
		break;
	default:
		reg |= SDC_CKGEN_SMPL_DLY_8;
		break;
	}

	sunxi_mmc_writel(host, SDC_SMPL_DL_CT, reg);
}

/* Get the clock index for a given timing */
static unsigned int sunxi_mmc_get_timing_clk(unsigned int timing)
{
	switch (timing) {
	case MMC_TIMING_LEGACY:
		return SUNXI_CLK_25M;
	case MMC_TIMING_MMC_HS:
	case MMC_TIMING_SD_HS:
	case MMC_TIMING_UHS_SDR12:
		return SUNXI_CLK_50M;
	case MMC_TIMING_UHS_SDR25:
		return SUNXI_CLK_50M;
	case MMC_TIMING_UHS_SDR50:
	case MMC_TIMING_MMC_HS200:
		return SUNXI_CLK_104M;
	case MMC_TIMING_UHS_SDR104:
	case MMC_TIMING_MMC_HS400:
	case MMC_TIMING_MMC_HS400_ES:
		return SUNXI_CLK_208M;
	case MMC_TIMING_UHS_DDR50:
		return SUNXI_CLK_52M;
	default:
		return SUNXI_CLK_25M;
	}
}

/* Set the bus timing for the controller */
static void sunxi_mmc_set_bus_timing(struct sunxi_mmc_host *host,
				     unsigned int timing)
{
	u32 reg;

	host->timing = timing;

	reg = sunxi_mmc_readl(host, SDC_CTRL);
	reg &= ~(SDC_CTRL_FACTOR_MASK | SDC_CTRL_BURST_LEN_MASK);

	/* Set DMA burst length to 16 */
	reg |= SDC_CTRL_BURST_LEN_16;
	sunxi_mmc_writel(host, SDC_CTRL, reg);

	/* Set sample delay for the timing */
	sunxi_mmc_set_sample_delay(host, timing);

	/* Set bus width if applicable */
	reg = sunxi_mmc_readl(host, SDC_WIDTH);
	reg &= ~(SDC_WIDTH_CARD_WIDTH_8 | SDC_WIDTH_CARD_WIDTH_4 |
		 SDC_WIDTH_CARD_WIDTH_1);
}

/* Enable interrupt bits */
static void sunxi_mmc_enable_int(struct sunxi_mmc_host *host, u32 ints)
{
	sunxi_mmc_writel(host, SDC_INTEN, ints);
}

/* Clear interrupt status */
static void sunxi_mmc_clear_int(struct sunxi_mmc_host *host, u32 ints)
{
	sunxi_mmc_writel(host, SDC_INTSTS, ints);
}

/* Read FIFO data (PIO mode) */
static void sunxi_mmc_read_data_pio(struct sunxi_mmc_host *host,
				    struct scatterlist *sg)
{
	unsigned int remaining = sg->length;
	u32 *buf;

	buf = sg_virt(sg);

	while (remaining >= 4 && !(sunxi_mmc_readl(host, SDC_STATUS) &
				    SDC_STATUS_FIFO_EMPTY)) {
		*buf++ = sunxi_mmc_readl(host, SDC_DATA);
		remaining -= 4;
	}
}

/* Write FIFO data (PIO mode) */
static void sunxi_mmc_write_data_pio(struct sunxi_mmc_host *host,
				     struct scatterlist *sg)
{
	unsigned int remaining = sg->length;
	u32 *buf;

	buf = sg_virt(sg);

	while (remaining >= 4 && !(sunxi_mmc_readl(host, SDC_STATUS) &
				    SDC_STATUS_FIFO_FULL)) {
		sunxi_mmc_writel(host, SDC_DATA, *buf++);
		remaining -= 4;
	}
}

/* Set up DMA descriptor for transfer */
static int sunxi_mmc_dma_setup(struct sunxi_mmc_host *host,
			       struct mmc_data *data)
{
	struct scatterlist *sg;
	unsigned int i, len;
	int dir;
	u32 cfg;

	if (data->flags & MMC_DATA_READ) {
		dir = DMA_FROM_DEVICE;
		cfg = DMA_DESC_VALID | DMA_DESC_INT | DMA_DESC_LD;
	} else {
		dir = DMA_TO_DEVICE;
		cfg = DMA_DESC_VALID | DMA_DESC_INT | DMA_DESC_LD;
	}

	/* Count scatter-gather segments */
	for_each_sg(data->sg, sg, data->sg_len, i) {
		if (sg->length > SUNXI_DMA_MAX_LEN)
			return -EINVAL;
	}

	host->dma_len = dma_map_sg_attrs(host->dev, data->sg,
					 data->sg_len, dir,
					 DMA_ATTR_SKIP_CPU_SYNC);
	if (!host->dma_len)
		return -ENOMEM;

	/* Set DMA address and enable DMA */
	sunxi_mmc_writel(host, SDC_DMAC_ADDR, sg_dma_address(data->sg));

	len = sg->length;
	sunxi_mmc_writel(host, SDC_BLOCK, len / 512);

	/* Configure DMA controller */
	cfg = SDC_DMAC_EN | SDC_DMAC_MODE | SDC_DMAC_DE |
	      SDC_DMAC_BURST_16;
	sunxi_mmc_writel(host, SDC_DMAC, cfg);

	return 0;
}

/* Complete a DMA transfer */
static void sunxi_mmc_dma_complete(struct sunxi_mmc_host *host,
				   struct mmc_data *data)
{
	int dir = (data->flags & MMC_DATA_READ) ?
		DMA_FROM_DEVICE : DMA_TO_DEVICE;

	sunxi_mmc_clr_bit(host, SDC_DMAC, SDC_DMAC_EN);
	dma_unmap_sg_attrs(host->dev, data->sg,
			   data->sg_len, dir,
			   DMA_ATTR_SKIP_CPU_SYNC);
	host->dma_len = 0;
}

/* Send a command to the controller */
static int sunxi_mmc_send_cmd(struct mmc_host *mmc, struct mmc_command *cmd,
			      struct mmc_data *data)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	u32 intmask = SDC_INTSTS_CMD_DONE;
	u32 cmd_reg;
	int ret = 0;
	unsigned long timeout;

	host->cmd_done = false;
	host->error = false;
	host->data_done = !data;

	/* Set up command register */
	cmd_reg = (cmd->opcode << 24) | SDC_CMD_LOAD;

	if (cmd->flags & MMC_RSP_PRESENT) {
		cmd_reg |= SDC_CMD_RESP_EXP;
		if (cmd->flags & MMC_RSP_CRC)
			cmd_reg |= SDC_CMD_RESP_CRC;
	}

	if (data) {
		cmd_reg |= SDC_CMD_DATA;
		host->blen = data->blen;
		host->rlen = data->blocks * data->blen;
	}

	/* Set timeout */
	sunxi_mmc_writel(host, SDC_TMOUT,
			 SDC_TMOUT_DATA(SUNXI_DEFAULT_TIMEOUT) |
			 SDC_TMOUT_RESP(SUNXI_DEFAULT_TIMEOUT));

	/* Clear pending interrupts */
	sunxi_mmc_clear_int(host, SDC_INTSTS_ERROR | SDC_INTSTS_CMD_DONE |
			    SDC_INTSTS_TX_DONE | SDC_INTSTS_RX_DONE);

	/* Enable command done interrupt */
	sunxi_mmc_enable_int(host, SDC_INTSTS_CMD_DONE |
			     SDC_INTSTS_ERROR);

	/* Write command argument */
	sunxi_mmc_writel(host, SDC_ARG, cmd->arg);

	/* Write command register (triggers the command) */
	sunxi_mmc_writel(host, SDC_CMD, cmd_reg);

	/* Wait for command completion */
	timeout = jiffies + msecs_to_jiffies(1000);
	while (time_is_before_jiffies(timeout)) {
		if (host->cmd_done)
			break;
		cond_resched();
	}

	if (!host->cmd_done) {
		dev_err(host->dev, "Command %d timeout\n", cmd->opcode);
		ret = -ETIMEDOUT;
		goto out;
	}

	if (host->error) {
		ret = -EIO;
		goto out;
	}

	/* Read response registers */
	if (cmd->flags & MMC_RSP_PRESENT) {
		if (cmd->flags & MMC_RSP_136) {
			cmd->resp[0] = sunxi_mmc_readl(host, SDC_RESP1);
			cmd->resp[1] = sunxi_mmc_readl(host, SDC_RESP0);
			cmd->resp[2] = sunxi_mmc_readl(host, SDC_RESP3);
			cmd->resp[3] = sunxi_mmc_readl(host, SDC_RESP2);
		} else {
			cmd->resp[0] = sunxi_mmc_readl(host, SDC_RESP0);
		}
	}

out:
	sunxi_mmc_clear_int(host, SDC_INTSTS_CMD_DONE | SDC_INTSTS_ERROR);
	return ret;
}

/* Handle data transfer (PIO or DMA) */
static int sunxi_mmc_transfer_data(struct mmc_host *mmc,
				   struct mmc_data *data)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	struct scatterlist *sg;
	unsigned int i;
	u32 intmask;
	int ret = 0;
	unsigned long timeout;

	if (host->xfer_mode == SUNXI_XFER_DMA_DESC) {
		ret = sunxi_mmc_dma_setup(host, data);
		if (ret) {
			dev_err(host->dev, "DMA setup failed: %d\n", ret);
			return ret;
		}

		/* Wait for DMA completion */
		intmask = SDC_INTSTS_TX_DONE | SDC_INTSTS_RX_DONE |
			  SDC_INTSTS_ERROR;
		sunxi_mmc_enable_int(host, intmask);
		sunxi_mmc_clear_int(host, intmask);

		timeout = jiffies + msecs_to_jiffies(1000);
		while (!host->dma_done && !host->error &&
		       time_is_before_jiffies(timeout))
			cond_resched();

		sunxi_mmc_dma_complete(host, data);

		if (host->error)
			ret = -EIO;
	} else {
		/* PIO transfer */
		for_each_sg(data->sg, sg, data->sg_len, i) {
			timeout = jiffies + msecs_to_jiffies(1000);

			if (data->flags & MMC_DATA_READ) {
				while (time_is_before_jiffies(timeout)) {
					if (sunxi_mmc_readl(host, SDC_STATUS) &
					    SDC_STATUS_FIFO_EMPTY)
						continue;
					sunxi_mmc_read_data_pio(host, sg);
					break;
				}
			} else {
				while (time_is_before_jiffies(timeout)) {
					if (sunxi_mmc_readl(host, SDC_STATUS) &
					    SDC_STATUS_FIFO_FULL)
						continue;
					sunxi_mmc_write_data_pio(host, sg);
					break;
				}
			}
		}
	}

	return ret;
}

/* Set the MMC host's I/O configuration (power, bus width, timing) */
static void sunxi_mmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	u32 reg;

	switch (ios->power_mode) {
	case MMC_POWER_UP:
		sunxi_mmc_clks_enable(host);
		sunxi_mmc_init_host(host);
		break;
	case MMC_POWER_ON:
		break;
	case MMC_POWER_OFF:
		sunxi_mmc_clks_disable(host);
		return;
	default:
		break;
	}

	/* Set bus width */
	reg = sunxi_mmc_readl(host, SDC_WIDTH);
	reg &= ~(SDC_WIDTH_CARD_WIDTH_8 | SDC_WIDTH_CARD_WIDTH_4 |
		 SDC_WIDTH_CARD_WIDTH_1);

	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_8:
		reg |= SDC_WIDTH_CARD_WIDTH_8;
		break;
	case MMC_BUS_WIDTH_4:
		reg |= SDC_WIDTH_CARD_WIDTH_4;
		break;
	default:
		reg |= SDC_WIDTH_CARD_WIDTH_1;
		break;
	}
	sunxi_mmc_writel(host, SDC_WIDTH, reg);

	/* Set clock */
	if (ios->clock) {
		sunxi_mmc_set_clk(host, ios->clock);
		sunxi_mmc_set_bus_timing(host, ios->timing);
	}

	/* Set signal voltage */
	switch (ios->signal_voltage) {
	case MMC_SIGNAL_VOLTAGE_180:
		/* TODO: Regulator control for 1.8V */
		host->signal_voltage = MMC_SIGNAL_VOLTAGE_180;
		break;
	case MMC_SIGNAL_VOLTAGE_330:
		/* TODO: Regulator control for 3.3V */
		host->signal_voltage = MMC_SIGNAL_VOLTAGE_330;
		break;
	default:
		break;
	}
}

/* Get write protect status */
static int sunxi_mmc_get_ro(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	if (!gpio_is_valid(host->wp_gpio))
		return 0;

	return !gpio_get_value_cansleep(host->wp_gpio);
}

/* Get card detect status */
static int sunxi_mmc_get_cd(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	if (!gpio_is_valid(host->cd_gpio))
		return 1;  /* Assume card present if no CD GPIO */

	return !gpio_get_value_cansleep(host->cd_gpio);
}

/* Enable or disable card detection */
static void sunxi_mmc_enable_card_detection(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	if (gpio_is_valid(host->cd_gpio)) {
		u32 reg;

		reg = sunxi_mmc_readl(host, SDC_PRGPIO);
		if (!sunxi_mmc_get_cd(mmc))
			reg |= SDC_CTRL_GPIO_PRESENT;
		else
			reg &= ~SDC_CTRL_GPIO_PRESENT;
		sunxi_mmc_writel(host, SDC_PRGPIO, reg);
	}
}

/* Handle the CMD52 (SDIO) command */
static int sunxi_mmc_sdio_send_cmd(struct mmc_host *mmc,
				   struct mmc_command *cmd)
{
	return sunxi_mmc_send_cmd(mmc, cmd, NULL);
}

/* IRQ handler for the MMC controller */
static irqreturn_t sunxi_mmc_irq(int irq, void *dev_id)
{
	struct sunxi_mmc_host *host = dev_id;
	u32 intsts;

	intsts = sunxi_mmc_readl(host, SDC_INTSTS);

	/* Acknowledge all pending interrupts */
	sunxi_mmc_clear_int(host, intsts);

	if (intsts & SDC_INTSTS_ERROR) {
		host->error = true;
		sunxi_mmc_clr_bit(host, SDC_INTEN, SDC_INTSTS_ERROR);
	}

	if (intsts & SDC_INTSTS_CMD_DONE) {
		host->cmd_done = true;
		sunxi_mmc_clr_bit(host, SDC_INTEN, SDC_INTSTS_CMD_DONE);
	}

	if (intsts & (SDC_INTSTS_TX_DONE | SDC_INTSTS_RX_DONE)) {
		host->dma_done = true;
		sunxi_mmc_clr_bit(host, SDC_INTEN,
				   SDC_INTSTS_TX_DONE | SDC_INTSTS_RX_DONE);
	}

	if (intsts & SDC_INTSTS_CARDInserted) {
		host->card_present = true;
		sunxi_mmc_enable_card_detection(host->mmc);
		mmc_detect_change(host->mmc, 0);
	}

	if (intsts & SDC_INTSTS_CARDRemoved) {
		host->card_present = false;
		sunxi_mmc_enable_card_detection(host->mmc);
		mmc_detect_change(host->mmc, 0);
	}

	return IRQ_HANDLED;
}

/* Initialize the MMC host controller */
static int sunxi_mmc_init_host(struct sunxi_mmc_host *host)
{
	u32 reg;
	int ret;

	/* Reset the controller */
	ret = sunxi_mmc_reset(host, 0);
	if (ret) {
		dev_err(host->dev, "Failed to reset controller: %d\n", ret);
		return ret;
	}

	/* Enable the controller */
	reg = sunxi_mmc_readl(host, SDC_CTRL);
	reg |= SDC_CTRL_EN;
	sunxi_mmc_writel(host, SDC_CTRL, reg);

	/* Set default clock divider */
	sunxi_mmc_writel(host, SDC_CLKDIV, 0);

	/* Set default timeout */
	sunxi_mmc_writel(host, SDC_TMOUT,
			 SDC_TMOUT_DATA(SUNXI_DEFAULT_TIMEOUT) |
			 SDC_TMOUT_RESP(SUNXI_DEFAULT_TIMEOUT));

	/* Clear all interrupts */
	sunxi_mmc_clear_int(host, 0xffffffff);

	/* Enable card detection interrupts */
	sunxi_mmc_enable_int(host, SDC_INTSTS_CARDInserted |
			     SDC_INTSTS_CARDRemoved);

	/* Configure DMA burst length and enable */
	reg = sunxi_mmc_readl(host, SDC_CTRL);
	reg &= ~SDC_CTRL_BURST_LEN_MASK;
	reg |= SDC_CTRL_BURST_LEN_16;
	sunxi_mmc_writel(host, SDC_CTRL, reg);

	/* Set sample delay to safe default */
	sunxi_mmc_set_sample_delay(host, MMC_TIMING_LEGACY);

	return 0;
}

/* Card detection work */
static void sunxi_mmc_card_detection_work(struct work_struct *work)
{
	struct sunxi_mmc_host *host =
		container_of(work, struct sunxi_mmc_host, work);
	int present;

	present = sunxi_mmc_get_cd(host->mmc);
	if (present != host->card_present) {
		host->card_present = present;
		sunxi_mmc_enable_card_detection(host->mmc);
		mmc_detect_change(host->mmc, msecs_to_jiffies(20));
	}
}

/* Timeout handler for card detect */
static void sunxi_mmc_timeout_handler(struct timer_list *t)
{
	struct sunxi_mmc_host *host = from_timer(host, t, detect_timer);

	schedule_work(&host->work);
	mod_timer(&host->detect_timer,
		  jiffies + msecs_to_jiffies(SUNXI_CARD_DETECT_INTERVAL_MS));
}

/* Parse device tree properties */
static int sunxi_mmc_parse_dt(struct device *dev, struct sunxi_mmc_host *host)
{
	int ret;

	/* Get GPIOs */
	host->cd_gpio = of_get_named_gpio(dev->of_node, "cd-gpios", 0);
	if (!gpio_is_valid(host->cd_gpio)) {
		dev_dbg(dev, "No CD GPIO, using internal detection\n");
		host->cd_gpio = -ENOENT;
	} else {
		ret = devm_gpio_request_one(dev, host->cd_gpio,
					    GPIOF_DIR_IN, "mmc-cd");
		if (ret) {
			dev_err(dev, "Failed to request CD GPIO: %d\n", ret);
			return ret;
		}
	}

	host->wp_gpio = of_get_named_gpio(dev->of_node, "wp-gpios", 0);
	if (!gpio_is_valid(host->wp_gpio)) {
		dev_dbg(dev, "No WP GPIO\n");
		host->wp_gpio = -ENOENT;
	} else {
		ret = devm_gpio_request_one(dev, host->wp_gpio,
					    GPIOF_DIR_IN, "mmc-wp");
		if (ret) {
			dev_err(dev, "Failed to request WP GPIO: %d\n", ret);
			return ret;
		}
	}

	/* Determine transfer mode from DT */
	if (of_property_read_bool(dev->of_node, "allwinner,use-dma"))
		host->xfer_mode = SUNXI_XFER_DMA_DESC;
	else
		host->xfer_mode = SUNXI_XFER_PIO;

	/* Parse capabilities */
	host->capabilities = SUNXI_HOST_CAP_HS;

	if (of_property_read_bool(dev->of_node, "allwinner,hs200-cap"))
		host->capabilities2 = SUNXI_HOST_CAP_HS200;
	else
		host->capabilities2 = 0;

	if (of_property_read_bool(dev->of_node, "allwinner,hs400-cap"))
		host->capabilities2 |= SUNXI_HOST_CAP_HS400;

	return 0;
}

/* Allocate DMA descriptors */
static int sunxi_mmc_alloc_dma_desc(struct sunxi_mmc_host *host)
{
	host->dma_desc_list.num_desc = 256;
	host->dma_desc_list.desc = dma_alloc_coherent(host->dev,
		sizeof(struct sunxi_dma_desc) * host->dma_desc_list.num_desc,
		&host->dma_desc_list.desc_dma, GFP_KERNEL);

	if (!host->dma_desc_list.desc)
		return -ENOMEM;

	return 0;
}

static void sunxi_mmc_free_dma_desc(struct sunxi_mmc_host *host)
{
	if (host->dma_desc_list.desc) {
		dma_free_coherent(host->dev,
				  sizeof(struct sunxi_dma_desc) *
				  host->dma_desc_list.num_desc,
				  host->dma_desc_list.desc,
				  host->dma_desc_list.desc_dma);
		host->dma_desc_list.desc = NULL;
	}
}

/* Add a scatter-gather entry to the DMA descriptor chain */
static int sunxi_mmc_fill_dma_desc(struct sunxi_mmc_host *host,
				   struct scatterlist *sgl, int sg_len)
{
	struct scatterlist *sg;
	struct sunxi_dma_desc *desc;
	unsigned int i;

	for_each_sg(sgl, sg, sg_len, i) {
		if (i >= host->dma_desc_list.num_desc - 1)
			return -EINVAL;

		desc = &host->dma_desc_list.desc[i];
		desc->buf_addr = sg_dma_address(sg);
		desc->buf_size = sg->length;
		desc->next_desc_addr = host->dma_desc_list.desc_dma +
			((i + 1) * sizeof(struct sunxi_dma_desc));

		desc->config = DMA_DESC_VALID | DMA_DESC_INT;
	}

	/* Mark last descriptor as end of chain */
	desc = &host->dma_desc_list.desc[i - 1];
	desc->config |= DMA_DESC_LD;

	return 0;
}

/* Setup transfer data structures */
static int sunxi_mmc_prepare_data(struct mmc_host *mmc,
				  struct mmc_command *cmd,
				  struct mmc_data *data)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	int ret;

	if (!data)
		return 0;

	/* Check alignment */
	if (data->blocks * data->blen > SUNXI_DMA_MAX_LEN)
		return -EINVAL;

	if (host->xfer_mode == SUNXI_XFER_DMA_DESC) {
		ret = sunxi_mmc_dma_setup(host, data);
		if (ret)
			return ret;
	}

	return 0;
}

/* Finalize data transfer */
static void sunxi_mmc_finish_data(struct mmc_host *mmc,
				  struct mmc_data *data)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	if (!data)
		return;

	if (host->xfer_mode == SUNXI_XFER_DMA_DESC && host->dma_len)
		sunxi_mmc_dma_complete(host, data);
}

/* Request handler for the MMC core */
static void sunxi_mmc_request(struct mmc_host *mmc,
			      struct mmc_request *mrq)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	int ret;

	/* Prepare data transfer */
	ret = sunxi_mmc_prepare_data(mmc, mrq->cmd, mrq->data);
	if (ret) {
		mrq->cmd->error = ret;
		mmc_request_done(mmc, mrq);
		return;
	}

	/* Send the command */
	ret = sunxi_mmc_send_cmd(mmc, mrq->cmd, mrq->data);
	if (ret) {
		mrq->cmd->error = ret;
		goto out;
	}

	/* Transfer data if present */
	if (mrq->data) {
		ret = sunxi_mmc_transfer_data(mmc, mrq->data);
		if (ret) {
			mrq->data->error = ret;
		}
	}

out:
	/* Finalize data transfer */
	sunxi_mmc_finish_data(mmc, mrq->data);

	mmc_request_done(mmc, mrq);
}

/* Execute tuning for HS200/SDR104 */
static int sunxi_mmc_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	u32 reg;
	int ret;
	int sample;

	dev_dbg(host->dev, "Execute tuning for opcode %u\n", opcode);

	/* Try different sample delays */
	for (sample = 0; sample < 4; sample++) {
		reg = sunxi_mmc_readl(host, SDC_SMPL_DL_CT);
		reg &= ~SDC_CKGEN_SMPL_DLY_MASK;
		reg |= (sample << 20);
		sunxi_mmc_writel(host, SDC_SMPL_DL_CT, reg);

		ret = mmc_send_tuning(mmc, opcode, NULL);
		if (ret == 0) {
			dev_dbg(host->dev, "Tuning passed at sample %d\n",
				sample);
			return 0;
		}
	}

	dev_err(host->dev, "Tuning failed\n");
	return -EIO;
}

/* Card status query */
static int sunxi_mmc_card_busy(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	u32 status;

	status = sunxi_mmc_readl(host, SDC_STATUS);

	return !!(status & SDC_STATUS_CARD_DATA_BUSY);
}

/* Reset host */
static void sunxi_mmc_reset(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	sunxi_mmc_init_host(host);
}

/* HS400 enhanced strobe support */
static int sunxi_mmc_hs400_enhanced_strobe(struct mmc_host *mmc,
					   struct mmc_ios *ios)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	if (ios->enhanced_strobe) {
		dev_dbg(host->dev, "HS400 enhanced strobe enabled\n");
		sunxi_mmc_set_bus_timing(host, MMC_TIMING_MMC_HS400_ES);
	} else {
		sunxi_mmc_set_bus_timing(host, MMC_TIMING_MMC_HS400);
	}

	return 0;
}

static const struct mmc_host_ops sunxi_mmc_ops = {
	.request	= sunxi_mmc_request,
	.set_ios	= sunxi_mmc_set_ios,
	.get_ro		= sunxi_mmc_get_ro,
	.get_cd		= sunxi_mmc_get_cd,
	.enable_card_detection = sunxi_mmc_enable_card_detection,
	.execute_tuning	= sunxi_mmc_execute_tuning,
	.card_busy	= sunxi_mmc_card_busy,
	.hs400_enhanced_strobe = sunxi_mmc_hs400_enhanced_strobe,
	.start_signal_voltage_switch = NULL, /* TODO: voltage switching */
	.card_event	= NULL,
};

/* Driver probe */
static int sunxi_mmc_probe(struct platform_device *pdev)
{
	struct sunxi_mmc_host *host;
	struct mmc_host *mmc;
	struct resource *res;
	int ret;

	mmc = mmc_alloc_host(sizeof(struct sunxi_mmc_host), &pdev->dev);
	if (!mmc)
		return -ENOMEM;

	host = mmc_priv(mmc);
	host->mmc = mmc;
	host->dev = &pdev->dev;

	platform_set_drvdata(pdev, host);

	/* Map registers */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	host->reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(host->reg_base)) {
		ret = PTR_ERR(host->reg_base);
		goto err_free_host;
	}
	host->phys_base = res->start;

	/* Get IRQ */
	host->irq = platform_get_irq(pdev, 0);
	if (host->irq < 0) {
		ret = host->irq;
		goto err_free_host;
	}

	/* Get clocks */
	host->clk_mmc = devm_clk_get(&pdev->dev, "mmc");
	if (IS_ERR(host->clk_mmc)) {
		ret = dev_err_probe(&pdev->dev,
				    PTR_ERR(host->clk_mmc),
				    "Failed to get MMC clock\n");
		goto err_free_host;
	}

	host->clk_bus = devm_clk_get(&pdev->dev, "bus");
	if (IS_ERR(host->clk_bus)) {
		ret = dev_err_probe(&pdev->dev,
				    PTR_ERR(host->clk_bus),
				    "Failed to get bus clock\n");
		goto err_free_host;
	}

	host->clk_mod = devm_clk_get(&pdev->dev, "mod");
	if (IS_ERR(host->clk_mod)) {
		ret = dev_err_probe(&pdev->dev,
				    PTR_ERR(host->clk_mod),
				    "Failed to get module clock\n");
		goto err_free_host;
	}

	/* Get optional reset */
	host->reset = devm_reset_control_get_optional_shared(&pdev->dev,
							      NULL);
	if (IS_ERR(host->reset))
		host->reset = NULL;

	/* Parse DT properties */
	ret = sunxi_mmc_parse_dt(&pdev->dev, host);
	if (ret)
		goto err_free_host;

	/* Allocate DMA descriptors if DMA mode */
	if (host->xfer_mode == SUNXI_XFER_DMA_DESC) {
		ret = sunxi_mmc_alloc_dma_desc(host);
		if (ret)
			goto err_free_host;
	}

	/* Initialize wait queues */
	init_waitqueue_head(&host->incorrect_cmd);
	init_waitqueue_head(&host->incorrect_data);

	/* Initialize card detect work and timer */
	INIT_WORK(&host->work, sunxi_mmc_card_detection_work);
	timer_setup(&host->detect_timer, sunxi_mmc_timeout_handler, 0);

	/* Set up MMC host */
	mmc->ops = &sunxi_mmc_ops;
	mmc->f_min = 400000;
	mmc->f_max = 208000000;
	mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34 |
			 MMC_VDD_165_195;

	/* Set capabilities based on DT */
	mmc->caps = host->capabilities;
	mmc->caps2 = host->capabilities2;

	/* Set max segment size for DMA */
	mmc->max_seg_size = SUNXI_DMA_MAX_LEN;
	mmc->max_segs = host->xfer_mode == SUNXI_XFER_DMA_DESC ?
			host->dma_desc_list.num_desc : 1;
	mmc->max_blk_size = 512;
	mmc->max_blk_count = 65535;
	mmc->max_req_size = SUNXI_DMA_MAX_LEN;

	/* Enable 4-bit bus by default, 8-bit if supported */
	if (mmc->caps & MMC_CAP_8_BIT_DATA) {
		mmc->caps |= MMC_CAP_8_BIT_DATA;
		mmc->caps |= MMC_CAP_4_BIT_DATA;
	} else {
		mmc->caps |= MMC_CAP_4_BIT_DATA;
	}

	/* Request IRQ */
	ret = devm_request_irq(&pdev->dev, host->irq, sunxi_mmc_irq, 0,
			       DRIVER_NAME, host);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request IRQ %d: %d\n",
			host->irq, ret);
		goto err_free_dma;
	}

	/* Add the MMC host */
	ret = mmc_add_host(mmc);
	if (ret) {
		dev_err(&pdev->dev, "Failed to add MMC host: %d\n", ret);
		goto err_free_dma;
	}

	/* Start card detection */
	host->card_present = true;
	sunxi_mmc_enable_card_detection(mmc);
	mod_timer(&host->detect_timer,
		  jiffies + msecs_to_jiffies(SUNXI_CARD_DETECT_INTERVAL_MS));

	dev_info(&pdev->dev, "sun60i-a733 MMC controller at 0x%pa, irq %d\n",
		 &host->phys_base, host->irq);

	return 0;

err_free_dma:
	cancel_work_sync(&host->work);
	del_timer_sync(&host->detect_timer);
	if (host->xfer_mode == SUNXI_XFER_DMA_DESC)
		sunxi_mmc_free_dma_desc(host);
err_free_host:
	mmc_free_host(mmc);
	return ret;
}

/* Driver remove */
static int sunxi_mmc_remove(struct platform_device *pdev)
{
	struct sunxi_mmc_host *host = platform_get_drvdata(pdev);
	struct mmc_host *mmc = host->mmc;

	del_timer_sync(&host->detect_timer);
	cancel_work_sync(&host->work);

	mmc_remove_host(mmc);

	/* Disable clocks */
	sunxi_mmc_clks_disable(host);

	if (host->xfer_mode == SUNXI_XFER_DMA_DESC)
		sunxi_mmc_free_dma_desc(host);

	mmc_free_host(mmc);

	return 0;
}

/* Suspend support */
static int __maybe_unused sunxi_mmc_suspend(struct device *dev)
{
	struct sunxi_mmc_host *host = dev_get_drvdata(dev);
	struct mmc_host *mmc = host->mmc;
	int ret = 0;

	del_timer_sync(&host->detect_timer);
	cancel_work_sync(&host->work);

	if (mmc->card) {
		ret = mmc_suspend_host(mmc);
		if (ret)
			goto out;
	}

	sunxi_mmc_clks_disable(host);

out:
	return ret;
}

/* Resume support */
static int __maybe_unused sunxi_mmc_resume(struct device *dev)
{
	struct sunxi_mmc_host *host = dev_get_drvdata(dev);
	struct mmc_host *mmc = host->mmc;
	int ret;

	ret = sunxi_mmc_clks_enable(host);
	if (ret) {
		dev_err(dev, "Failed to enable clocks: %d\n", ret);
		return ret;
	}

	ret = sunxi_mmc_init_host(host);
	if (ret) {
		dev_err(dev, "Failed to init host: %d\n", ret);
		sunxi_mmc_clks_disable(host);
		return ret;
	}

	if (mmc->card) {
		ret = mmc_resume_host(mmc);
		if (ret)
			return ret;
	}

	mod_timer(&host->detect_timer,
		  jiffies + msecs_to_jiffies(SUNXI_CARD_DETECT_INTERVAL_MS));

	return 0;
}

static SIMPLE_DEV_PM_OPS(sunxi_mmc_pm_ops,
			  sunxi_mmc_suspend, sunxi_mmc_resume);

static const struct of_device_id sunxi_mmc_of_match[] = {
	{
		.compatible = "allwinner,sun60i-a733-mmc",
	}, {
		.compatible = "allwinner,sun20i-d1-mmc",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, sunxi_mmc_of_match);

static struct platform_driver sunxi_mmc_driver = {
	.probe		= sunxi_mmc_probe,
	.remove		= sunxi_mmc_remove,
	.driver		= {
		.name	= DRV_NAME,
		.of_match_table = sunxi_mmc_of_match,
		.pm	= &sunxi_mmc_pm_ops,
	},
};
module_platform_driver(sunxi_mmc_driver);

MODULE_DESCRIPTION("Allwinner sun60i-a733 MMC host driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Allwinner Technology Co., Ltd.");
MODULE_ALIAS("platform:sun20i-d1-mmc");
