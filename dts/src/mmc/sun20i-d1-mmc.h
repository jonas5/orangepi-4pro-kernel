/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Header for Allwinner sun60i-a733 MMC controller driver.
 *
 * Copyright (C) 2024 Allwinner Technology Co., Ltd.
 */

#ifndef _SUN20I_D1_MMC_H_
#define _SUN20I_D1_MMC_H_

/* SDC register offsets */
#define SDC_CTRL		0x00
#define SDC_CLKDIV		0x04
#define SDC_CKGEN		0x08
#define SDC_TMOUT		0x0c
#define SDC_WIDTH		0x10
#define SDC_DATA		0x20
#define SDC_BLOCK		0x24
#define SDC_FIFOCT		0x28
#define SDC_INTSTS		0x30
#define SDC_INTEN		0x34
#define SDC_CMD		0x3c
#define SDC_ARG			0x40
#define SDC_RESP0		0x50
#define SDC_RESP1		0x54
#define SDC_RESP2		0x58
#define SDC_RESP3		0x5c
#define SDC_STATUS		0x60
#define SDC_FIFOSTA		0x64
#define SDC_DATASTS		0xa4
#define SDC_DMAADDR		0xc0
#define SDC_DMAC		0xc4
#define SDC_DMAC_ADDR		0xc8
#define SDC_DMA_INT		0xcc
#define SDC_DMA_INT_EN		0xd0
#define SDC_PRGPIO		0x38
#define SDC_DRV_DL		0x400
#define SDC_SAMPL_DL		0x404
#define SDC_DS_DL		0x408
#define SDC_SMPL_DL_CT		0x420
#define SDC_GCTRL		0x440

/* SDC_CTRL bits */
#define SDC_CTRL_STOP_CLK		BIT(31)
#define SDC_CTRL_ACCESS_DONE		BIT(30)
#define SDC_CTRL_GPIO_PRESENT		BIT(29)
#define SDC_CTRL_BURST_LEN_MASK		GENMASK(9, 8)
#define SDC_CTRL_BURST_LEN_16		(2 << 8)
#define SDC_CTRL_BURST_LEN_8		(1 << 8)
#define SDC_CTRL_BURST_LEN_4		(0 << 8)
#define SDC_CTRL_CARD_IS_EMMC		BIT(7)
#define SDC_CTRL_EN			BIT(2)
#define SDC_CTRL_FACTOR_MASK		GENMASK(1, 0)

/* SDC_CLKDIV bits */
#define SDC_CLKDIV_DIV2_MASK		GENMASK(15, 8)
#define SDC_CLKDIV_DIV1_MASK		GENMASK(7, 0)

/* SDC_CKGEN bits */
#define SDC_CKGEN_SMPL_DLY_MASK		GENMASK(25, 20)
#define SDC_CKGEN_SMPL_DLY_1		(0 << 20)
#define SDC_CKGEN_SMPL_DLY_2		(1 << 20)
#define SDC_CKGEN_SMPL_DLY_4		(2 << 20)
#define SDC_CKGEN_SMPL_DLY_8		(3 << 20)
#define SDC_CKGEN_OUT_MASK		GENMASK(1, 0)
#define SDC_CKGEN_OUT_TEST		BIT(2)

/* SDC_TMOUT bits */
#define SDC_TMOUT_DATA_MASK		GENMASK(31, 16)
#define SDC_TMOUT_DATA(x)		((x) << 16)
#define SDC_TMOUT_RESP_MASK		GENMASK(15, 8)
#define SDC_TMOUT_RESP(x)		((x) << 8)
#define SDC_TMOUT_BUSY			GENMASK(7, 0)

/* SDC_WIDTH bits */
#define SDC_WIDTH_CARD_WIDTH_8		BIT(24)
#define SDC_WIDTH_CARD_WIDTH_4		BIT(0)
#define SDC_WIDTH_CARD_WIDTH_1		0

/* SDC_INTSTS bits */
#define SDC_INTSTS_CARD_ERROR		BIT(22)
#define SDC_INTSTS_CARDInserted		BIT(21)
#define SDC_INTSTS_CARDRemoved		BIT(20)
#define SDC_INTSTS_DAT_BCRC_ERR		BIT(14)
#define SDC_INTSTS_DAT_CRC_ERR		BIT(13)
#define SDC_INTSTS_DAT_TIME_OUT		BIT(12)
#define SDC_INTSTS_RESP_BCRC_ERR	BIT(10)
#define SDC_INTSTS_RESP_CRC_ERR		BIT(9)
#define SDC_INTSTS_RESP_TIME_OUT	BIT(8)
#define SDC_INTSTS_CMD_DONE		BIT(2)
#define SDC_INTSTS_TX_DONE		BIT(1)
#define SDC_INTSTS_RX_DONE		BIT(0)
#define SDC_INTSTS_ERROR		(SDC_INTSTS_CARD_ERROR | \
					 SDC_INTSTS_DAT_BCRC_ERR | \
					 SDC_INTSTS_DAT_CRC_ERR | \
					 SDC_INTSTS_DAT_TIME_OUT | \
					 SDC_INTSTS_RESP_BCRC_ERR | \
					 SDC_INTSTS_RESP_CRC_ERR | \
					 SDC_INTSTS_RESP_TIME_OUT)

/* SDC_INTEN bits */
#define SDC_INTEN_CARD_ERROR		BIT(22)
#define SDC_INTEN_CARDInserted		BIT(21)
#define SDC_INTEN_CARDRemoved		BIT(20)
#define SDC_INTEN_DAT_BCRC_ERR		BIT(14)
#define SDC_INTEN_DAT_CRC_ERR		BIT(13)
#define SDC_INTEN_DAT_TIME_OUT		BIT(12)
#define SDC_INTEN_RESP_BCRC_ERR		BIT(10)
#define SDC_INTEN_RESP_CRC_ERR		BIT(9)
#define SDC_INTEN_RESP_TIME_OUT		BIT(8)
#define SDC_INTEN_CMD_DONE		BIT(2)
#define SDC_INTEN_TX_DONE		BIT(1)
#define SDC_INTEN_RX_DONE		BIT(0)

/* SDC_CMD bits */
#define SDC_CMD_LOAD			BIT(31)
#define SDC_CMD_RESP_EXP		BIT(6)
#define SDC_CMD_RESP_CRC		BIT(5)
#define SDC_CMD_DATA			BIT(4)
#define SDC_CMD_CMD_IDX_MASK		GENMASK(29, 24)

/* SDC_STATUS bits */
#define SDC_STATUS_FIFO_FULL		BIT(2)
#define SDC_STATUS_FIFO_EMPTY		BIT(3)
#define SDC_STATUS_CARD_DATA_BUSY	BIT(9)
#define SDC_STATUS_CARD_PRESENT		BIT(8)

/* SDC_FIFOSTA bits */
#define SDC_FIFOSTA_FIFO_LEFT_MASK	GENMASK(24, 16)
#define SDC_FIFOSTA_FIFO_LEFT(x)	(((x) >> 16) & 0x1ff)
#define SDC_FIFOSTA_FIFO_BLKS_MASK	GENMASK(8, 0)
#define SDC_FIFOSTA_FIFO_BLKS(x)	((x) & 0x1ff)

/* SDC_DATASTS bits */
#define SDC_DATASTS_FIFO_LEVEL_MASK	GENMASK(22, 16)
#define SDC_DATASTS_FIFO_LEVEL(x)	(((x) >> 16) & 0x7f)
#define SDC_DATASTS_ERR_NO_DATA		(0 << 12)
#define SDC_DATASTS_ERR_DATA		(1 << 12)
#define SDC_DATASTS_BLKS_DONE_MASK	GENMASK(11, 0)

/* SDC_DMAC bits */
#define SDC_DMAC_EN			BIT(0)
#define SDC_DMAC_MODE			BIT(1)
#define SDC_DMAC_DE			BIT(2)
#define SDC_DMAC_BURST_MASK		GENMASK(5, 4)
#define SDC_DMAC_BURST_16		(3 << 4)
#define SDC_DMAC_BURST_8		(2 << 4)
#define SDC_DMAC_BURST_4		(1 << 4)
#define SDC_DMAC_BURST_1		(0 << 4)
#define SDC_DMAC_FINISH			BIT(31)
#define SDC_DMAC_BUSY			BIT(30)
#define SDC_DMAC_ERR			BIT(15)

/* DMA descriptor structure for enhanced mode */
struct sunxi_dma_desc {
	u32	config;
	u32	buf_size;
	u32	buf_addr;
	u32	next_desc_addr;
};

/* DMA descriptor config bits */
#define DMA_DESC_VALID			BIT(31)
#define DMA_DESC_OWN			BIT(30)
#define DMA_DESC_INT			BIT(8)
#define DMA_DESC_LD			BIT(0)

/* DMA descriptor chain config */
struct sunxi_dma_desc_list {
	struct sunxi_dma_desc	*desc;
	dma_addr_t		desc_dma;
	unsigned int		num_desc;
};

/* Transfer mode */
enum sunxi_xfer_mode {
	SUNXI_XFER_PIO = 0,
	SUNXI_XFER_DMA_DESC,
};

/* Driver private data */
struct sunxi_mmc_host {
	struct	mmc_host		*mmc;
	struct	device			*dev;
	void __iomem			*reg_base;
	phys_addr_t			phys_base;
	int				irq;

	/* Clock and reset */
	struct clk			*clk_mmc;
	struct clk			*clk_bus;
	struct clk			*clk_mod;
	struct reset_control		*reset;

	/* DMA */
	enum sunxi_xfer_mode		xfer_mode;
	struct sunxi_dma_desc_list	dma_desc_list;
	void				*dma_buf;
	dma_addr_t			dma_addr;
	unsigned int			dma_len;

	/* Timing and mode */
	enum mmc_timing			timing;
	enum mmc_signal_voltage	signal_voltage;
	bool			.GetById;
	bool				def_speed_mode;

	/* Card state */
	bool				card_present;
	bool				wp_enabled;
	int				wp_gpio;
	int				cd_gpio;

	/* Transfer state */
	unsigned int			intSTS;
	unsigned int			blen;
	unsigned int			rlen;
	bool				dma_done;
	bool				cmd_done;
	bool				data_done;
	bool				error;
	wait_queue_head_t		incorrect_cmd;
	wait_queue_head_t		incorrect_data;

	/* Capabilities from DT */
	u32				capabilities;
	u32				capabilities2;

	/* Clock divider state */
	unsigned int			fclk_div;
	unsigned int			using_52m_clk;
};

/* Clock source IDs */
enum sunxi_clk_src {
	SUNXI_CLK_SRC_NONE = 0,
	SUNXI_CLK_SRC_HOSC,
	SUNXI_CLK_SRC_400M,
	SUNXI_CLK_SRC_24M,
};

/* Timing definitions for sunxi controller */
enum sunxi_timing {
	SUNXI_TIMING_LEGACY = 0,
	SUNXI_TIMING_MMC_HS,
	SUNXI_TIMING_SD_HS,
	SUNXI_TIMING_UHS_SDR12,
	SUNXI_TIMING_UHS_SDR25,
	SUNXI_TIMING_UHS_SDR50,
	SUNXI_TIMING_UHS_SDR104,
	SUNXI_TIMING_UHS_DDR50,
	SUNXI_TIMING_MMC_HS200,
	SUNXI_TIMING_MMC_HS400,
	SUNXI_TIMING_MMC_HS400_ES,
};

/* Bus speed mode register values */
#define SUNXI_CLK_400K		0
#define SUNXI_CLK_25M		1
#define SUNXI_CLK_50M		2
#define SUNXI_CLK_52M		3
#define SUNXI_CLK_104M		4
#define SUNXI_CLK_208M		5

/* Clock divider table */
struct sunxi_clk_factor {
	u32	factor;
	u32	integer;
	u32	fraction;
};

/* Access delay type */
enum sunxi_access_delay {
	SUNXI_ACCESS_DELAY_LEGACY = 0,
	SUNXI_ACCESS_DELAY_SMPL,
};

/* Host capabilities */
#define SUNXI_HOST_CAP_HS		MMC_CAP_1_BIT_DATA | \
					MMC_CAP_4_BIT_DATA | \
					MMC_CAP_8_BIT_DATA | \
					MMC_CAP_MMC_HIGHSPEED | \
					MMC_CAP_SD_HIGHSPEED | \
					MMC_CAP_3_3V | \
					MMC_CAP_1_8V | \
					MMC_CAP_ERASE | \
					MMC_CAP_CMD23

#define SUNXI_HOST_CAP_HS200		MMC_CAP_UHS_SDR50 | \
					MMC_CAP_UHS_SDR104 | \
					MMC_CAP_MMC_HS200 | \
					MMC_CAP_1_8V

#define SUNXI_HOST_CAP_HS400		MMC_CAP_MMC_HS400 | \
					MMC_CAP_MMC_HS400_ES | \
					MMC_CAP_1_8V

/* Default timeout values */
#define SUNXI_DEFAULT_TIMEOUT		0x03ffffff

#endif /* _SUN20I_D1_MMC_H_ */
