// SPDX-License-Identifier: GPL-2.0-only
/*
 * Allwinner A733 Clock Control Unit driver.
 *
 * The A733 CCU manages all module and bus clocks, as well as resets
 * for the A733 SoC (0x0200 2000, 2 KB register block).
 *
 * Modeled after other sunxi-ng CCU drivers (clk-sun50i-a523.c,
 * clk-sun50i-h616.c, etc.).
 *
 * -- Kconfig --
 *   config CLK_SUN60I_A733_CCU
 *   	tristate "Allwinner sun60i A733 CCU clock support"
 *   	depends on ARCH_SUNXI
 *   	default ARCH_SUNXI
 *   	select SUNXI_CCU
 *   	help
 *   	  Enable the clock control unit (CCU) driver for the Allwinner
 *   	  A733 SoC.  This driver provides clock gating, divider and
 *   	  reset support for all on-chip peripherals.
 *
 * -- Makefile --
 *   obj-$(CONFIG_CLK_SUN60I_A733_CCU) += clk-sun60i-a733.o
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_nkmp.h"
#include "ccu_phase.h"
#include "ccu_sdm.h"
#include "ccu_gate.h"
#include "ccu_div.h"
#include "ccu_mux.h"

#include <dt-bindings/clock/sun60i-a733-ccu.h>
#include <dt-bindings/clock/sun60i-a733-r-ccu.h>

/* ---------------------------------------------------------------------
 * Register map
 * --------------------------------------------------------------------- */

/* PLL control registers */
#define CCU_PLL_CPU_CTRL	0x000
#define CCU_PLL_AUDIO0_CTRL	0x004
#define CCU_PLL_VIDEO0_CTRL	0x008
#define CCU_PLL_VE_CTRL		0x00C
#define CCU_PLL_USB_CTRL	0x010
#define CCU_PLL_CPU_AXI_CTRL	0x014
#define CCU_PLL_PERIPH0_CTRL	0x018
#define CCU_PLL_PERIPH1_CTRL	0x01C
#define CCU_PLL_GPU_CTRL	0x020
#define CCU_PLL_DE_CTRL		0x024
#define CCU_PLL_PCIE_CTRL	0x028
#define CCU_PLL_DDR_CTRL	0x02C
#define CCU_PLL_AUDIO0X_CTRL	0x030
#define CCU_PLL_AUDIO0_2X_CTRL	0x034
#define CCU_PLL_AUDIO0_1X_CTRL	0x038
#define CCU_PLL_VIDEO0_2X_CTRL	0x03C
#define CCU_PLL_PERIPH0_2X_CTRL	0x040
#define CCU_PLL_PERIPH0_4X_CTRL	0x044

/* Clock divider / mux configuration registers */
#define CCU_CPU_AXI_CFG		0x050
#define CCU_AHB2_APB1_DIV	0x054
#define CCU_APB1_DIV_CFG	0x058
#define CCU_APB2_DIV_CFG	0x05C
#define CCU_PLL_CPU_AXI_DIV	0x048
#define CCU_PLL_CCI_DIV		0x04C
#define CCU_DSP_CFG		0x064

/* Bus gate + reset registers (24 bits: gate[31:24], reset[23:16]) */
#define CCU_UART_BGR		0x090
#define CCU_SPI_BGR		0x094
#define CCU_I2C_BGR		0x09C
#define CCU_USB_BGR		0x0A0
#define CCU_MMC_BGR		0x0A4
#define CCU_CE_BGR		0x0A8
#define CCU_DMA_BGR		0x0B0
#define CCU_THERMAL_BGR		0x0B4
#define CCU_HDMI_BGR		0x0B8
#define CCU_DE_BGR		0x0BC
#define CCU_GPU_BGR		0x0C0
#define CCU_GMAC_BGR		0x0C8
#define CCU_DAUDIO_BGR		0x0C4
#define CCU_OWA_BGR		0x0CC
#define CCU_DMIC_BGR		0x0D0
#define CCU_AUDIOC_BGR		0x0D4
#define CCU_THS_BGR		0x0D8
#define CCU_GPADC_BGR		0x0DC
#define CCU_LRADC_BGR		0x0E0
#define CCU_CEC_BGR		0x0E4
#define CCU_PWM_BGR		0x0E8
#define CCU_SPINLOCK_BGR	0x0EC
#define CCU_DRC_BGR		0x0F0
#define CCU_MSGBOX_BGR		0x0F4
#define CCU_ROM_BGR		0x0F8

/* Module (functional) clock configuration registers */
#define CCU_UART0_CLK_CFG	0x060
#define CCU_UART1_CLK_CFG	0x064
#define CCU_UART2_CLK_CFG	0x068
#define CCU_UART3_CLK_CFG	0x06C
#define CCU_UART4_CLK_CFG	0x070
#define CCU_UART5_CLK_CFG	0x074

#define CCU_SPI0_CLK_CFG	0x040
#define CCU_SPI1_CLK_CFG	0x044
#define CCU_SPI2_CLK_CFG	0x048
#define CCU_SPI3_CLK_CFG	0x04C

#define CCU_I2C0_CLK_CFG	0x050
#define CCU_I2C1_CLK_CFG	0x054
#define CCU_I2C2_CLK_CFG	0x058
#define CCU_I2C3_CLK_CFG	0x05C
#define CCU_I2C4_CLK_CFG	0x060
#define CCU_I2C5_CLK_CFG	0x064

#define CCU_USB_CLK_CFG		0x0A0
#define CCU_USB_PHY_CFG		0x0E0
#define CCU_USB_REF_CLK		0x0E4
#define CCU_USB_SUSPEND_CLK	0x0E8

#define CCU_MMC0_CLK_CFG	0x088
#define CCU_MMC1_CLK_CFG	0x08C
#define CCU_MMC2_CLK_CFG	0x090

#define CCU_GMAC_CLK_CFG	0x0C8
#define CCU_GMAC_TX_CLK		0x0CC

#define CCU_HDMI_CLK_CFG	0x0B8
#define CCU_HDMI_DDC_CLK	0x0BC

#define CCU_DE_CLK_CFG		0x0B0
#define CCU_GPU_CLK_CFG		0x0B4

#define CCU_THS_CLK_CFG		0x120
#define CCU_GPADC_CLK_CFG	0x124
#define CCU_LRADC_CLK_CFG	0x128
#define CCU_CEC_CLK_CFG		0x12C
#define CCU_PWM_CLK_CFG		0x130

#define CCU_DAUDIO_CLK_CFG	0x140
#define CCU_OWA_CLK_CFG		0x144
#define CCU_DMIC_CLK_CFG	0x148
#define CCU_AUDIOC_CLK_CFG	0x14C

#define CCU_SPINLOCK_CLK	0x150
#define CCU_DRC_CLK_CFG		0x154
#define CCU_MSGBOX_CLK		0x158
#define CCU_ROM_CLK		0x15C

/* ---------------------------------------------------------------------
 * PLL definitions
 * --------------------------------------------------------------------- */

/* PLL_CPU – NKMP: N[17:16]+1, K[25:24]+1, M[21:20]+1, P[29:26]+1, EN=31 */
static struct ccu_nkmp pll_cpu_clk = {
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN(16, 8, 1),
	.k		= _SUNXI_CCU_MULT_MIN(24, 2, 0),
	.m		= _SUNXI_CCU_DIV_MIN(20, 2, 0),
	.p		= _SUNXI_CCU_DIV_MIN(26, 4, 0),
	.common		= {
		.reg		= CCU_PLL_CPU_CTRL,
		.hw.init	= CLK_HW_INIT("pll-cpu", "osc24m",
					&ccu_nkmp_ops,
					CLK_SET_RATE_UNGATED),
	},
};

/* PLL_AUDIO0 – SDM (fractional / spread-spectrum PLL) */
static struct ccu_sdm pll_audio0_clk = {
	.enable		= BIT(31),
	.sdm		= _SUNXI_CCU_SDM_HELPER_WRITABLE(8, 16,
					sunxi_ccu_sdm_helper_get_rate,
					sunxi_ccu_sdm_helper_set_rate,
					sunxi_ccu_sdm_int_mul4),
	.common		= {
		.reg		= CCU_PLL_AUDIO0_CTRL,
		.hw.init	= CLK_HW_INIT("pll-audio0", "osc24m",
					&ccu_sdm_ops,
					CLK_SET_RATE_UNGATED),
	},
};

static SUNXI_CCU_GATE_HW(pll_audio0_4x_clk, "pll-audio0-4x",
			  "pll-audio0",
			  CCU_PLL_AUDIO0X_CTRL, BIT(31), 0);

static SUNXI_CCU_GATE_HW(pll_audio0_2x_clk, "pll-audio0-2x",
			  "pll-audio0-4x",
			  CCU_PLL_AUDIO0_2X_CTRL, BIT(31), 0);

static SUNXI_CCU_GATE_HW(pll_audio0_1x_clk, "pll-audio0-1x",
			  "pll-audio0-2x",
			  CCU_PLL_AUDIO0_1X_CTRL, BIT(31), 0);

/* PLL_VIDEO0 – NKMP */
static struct ccu_nkmp pll_video0_clk = {
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN(16, 8, 1),
	.k		= _SUNXI_CCU_MULT_MIN(24, 2, 0),
	.m		= _SUNXI_CCU_DIV_MIN(20, 2, 0),
	.p		= _SUNXI_CCU_DIV_MIN(26, 4, 0),
	.common		= {
		.reg		= CCU_PLL_VIDEO0_CTRL,
		.hw.init	= CLK_HW_INIT("pll-video0", "osc24m",
					&ccu_nkmp_ops,
					CLK_SET_RATE_UNGATED),
	},
};

static SUNXI_CCU_GATE_HW(pll_video0_2x_clk, "pll-video0-2x",
			  "pll-video0",
			  CCU_PLL_VIDEO0_2X_CTRL, BIT(31), 0);

/* PLL_VE – NKMP */
static struct ccu_nkmp pll_ve_clk = {
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN(16, 8, 1),
	.k		= _SUNXI_CCU_MULT_MIN(24, 2, 0),
	.m		= _SUNXI_CCU_DIV_MIN(20, 2, 0),
	.p		= _SUNXI_CCU_DIV_MIN(26, 4, 0),
	.common		= {
		.reg		= CCU_PLL_VE_CTRL,
		.hw.init	= CLK_HW_INIT("pll-ve", "osc24m",
					&ccu_nkmp_ops,
					CLK_SET_RATE_UNGATED),
	},
};

/* PLL_USB – fixed 480 MHz */
static SUNXI_CCU_GATE_HW(pll_usb_clk, "pll-usb", "osc24m",
			  CCU_PLL_USB_CTRL,
			  BIT(31) | BIT(30) | BIT(29) | BIT(24),
			  0);

/* PLL_CPU_AXI – NKMP */
static struct ccu_nkmp pll_cpu_axi_clk = {
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN(16, 8, 1),
	.k		= _SUNXI_CCU_MULT_MIN(24, 2, 0),
	.m		= _SUNXI_CCU_DIV_MIN(20, 2, 0),
	.p		= _SUNXI_CCU_DIV_MIN(26, 4, 0),
	.common		= {
		.reg		= CCU_PLL_CPU_AXI_CTRL,
		.hw.init	= CLK_HW_INIT("pll-cpu-axi", "osc24m",
					&ccu_nkmp_ops,
					CLK_SET_RATE_UNGATED),
	},
};

/* PLL_PERIPH0 – NKMP, two fixed output dividers (240/480 MHz) */
static struct ccu_nkmp pll_periph0_clk = {
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN(16, 8, 1),
	.k		= _SUNXI_CCU_MULT_MIN(24, 2, 0),
	.m		= _SUNXI_CCU_DIV_MIN(20, 2, 0),
	.p		= _SUNXI_CCU_DIV_MIN(26, 4, 0),
	.common		= {
		.reg		= CCU_PLL_PERIPH0_CTRL,
		.hw.init	= CLK_HW_INIT("pll-periph0", "osc24m",
					&ccu_nkmp_ops,
					CLK_SET_RATE_UNGATED),
	},
};

static SUNXI_CCU_GATE_HW(pll_periph0_2x_clk, "pll-periph0-2x",
			  "pll-periph0",
			  CCU_PLL_PERIPH0_2X_CTRL, BIT(31), 0);

static SUNXI_CCU_GATE_HW(pll_periph0_4x_clk, "pll-periph0-4x",
			  "pll-periph0",
			  CCU_PLL_PERIPH0_4X_CTRL, BIT(31), 0);

/* PLL_PERIPH1 – NKMP */
static struct ccu_nkmp pll_periph1_clk = {
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN(16, 8, 1),
	.k		= _SUNXI_CCU_MULT_MIN(24, 2, 0),
	.m		= _SUNXI_CCU_DIV_MIN(20, 2, 0),
	.p		= _SUNXI_CCU_DIV_MIN(26, 4, 0),
	.common		= {
		.reg		= CCU_PLL_PERIPH1_CTRL,
		.hw.init	= CLK_HW_INIT("pll-periph1", "osc24m",
					&ccu_nkmp_ops,
					CLK_SET_RATE_UNGATED),
	},
};

/* PLL_GPU – NKMP */
static struct ccu_nkmp pll_gpu_clk = {
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN(16, 8, 1),
	.k		= _SUNXI_CCU_MULT_MIN(24, 2, 0),
	.m		= _SUNXI_CCU_DIV_MIN(20, 2, 0),
	.p		= _SUNXI_CCU_DIV_MIN(26, 4, 0),
	.common		= {
		.reg		= CCU_PLL_GPU_CTRL,
		.hw.init	= CLK_HW_INIT("pll-gpu", "osc24m",
					&ccu_nkmp_ops,
					CLK_SET_RATE_UNGATED),
	},
};

/* PLL_DE – NKMP */
static struct ccu_nkmp pll_de_clk = {
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN(16, 8, 1),
	.k		= _SUNXI_CCU_MULT_MIN(24, 2, 0),
	.m		= _SUNXI_CCU_DIV_MIN(20, 2, 0),
	.p		= _SUNXI_CCU_DIV_MIN(26, 4, 0),
	.common		= {
		.reg		= CCU_PLL_DE_CTRL,
		.hw.init	= CLK_HW_INIT("pll-de", "osc24m",
					&ccu_nkmp_ops,
					CLK_SET_RATE_UNGATED),
	},
};

/* PLL_PCIE – NKMP */
static struct ccu_nkmp pll_pcie_clk = {
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN(16, 8, 1),
	.k		= _SUNXI_CCU_MULT_MIN(24, 2, 0),
	.m		= _SUNXI_CCU_DIV_MIN(20, 2, 0),
	.p		= _SUNXI_CCU_DIV_MIN(26, 4, 0),
	.common		= {
		.reg		= CCU_PLL_PCIE_CTRL,
		.hw.init	= CLK_HW_INIT("pll-pcie", "osc24m",
					&ccu_nkmp_ops,
					CLK_SET_RATE_UNGATED),
	},
};

/* PLL_DDR – NKMP */
static struct ccu_nkmp pll_ddr_clk = {
	.enable		= BIT(31),
	.n		= _SUNXI_CCU_MULT_MIN(16, 8, 1),
	.k		= _SUNXI_CCU_MULT_MIN(24, 2, 0),
	.m		= _SUNXI_CCU_DIV_MIN(20, 2, 0),
	.p		= _SUNXI_CCU_DIV_MIN(26, 4, 0),
	.common		= {
		.reg		= CCU_PLL_DDR_CTRL,
		.hw.init	= CLK_HW_INIT("pll-ddr", "osc24m",
					&ccu_nkmp_ops,
					CLK_SET_RATE_UNGATED),
	},
};

/* ---------------------------------------------------------------------
 * Clock parent tables
 * --------------------------------------------------------------------- */

static const char *const cpu_parents[] = {
	"osc24m", "pll-cpu", "pll-cpu-axi", "iosc"
};

static const char *const ahb_parents[] = {
	"osc24m", "pll-periph0-2x", "pll-periph0-4x", "iosc"
};

static const char *const apb1_parents[] = {
	"osc24m", "pll-periph0-2x", "pll-periph0-4x", "iosc"
};

static const char *const apb2_parents[] = {
	"osc24m", "pll-periph0-2x", "pll-periph0-4x", "iosc"
};

static const char *const mmc_parents[] = {
	"osc24m", "pll-periph0-2x", "pll-periph0-4x", "pll-periph1"
};

static const char *const de_parents[] = {
	"pll-de", "pll-periph0-2x", "pll-video0-2x", "pll-periph1"
};

static const char *const gpu_parents[] = {
	"pll-gpu", "pll-periph0-2x", "pll-video0-2x", "pll-periph1"
};

static const char *const hdmi_parents[] = {
	"pll-video0-2x", "pll-periph0-2x", "pll-periph1", "pll-periph0-4x"
};

static const char *const usb_parents[] = {
	"pll-periph0-2x", "pll-periph0-4x"
};

static const char *const audio_parents[] = {
	"pll-audio0-4x", "pll-audio0-2x", "pll-audio0-1x"
};

static const char *const uart_parents[] = {
	"osc24m", "pll-periph0-2x", "pll-periph0-4x"
};

static const char *const spi_parents[] = {
	"osc24m", "pll-periph0-2x", "pll-periph0-4x", "pll-periph1"
};

static const char *const i2c_parents[] = {
	"osc24m", "pll-periph0-2x", "pll-periph0-4x", "pll-periph1"
};

/* ---------------------------------------------------------------------
 * Bus / AHB / APB clocks
 * --------------------------------------------------------------------- */

/*
 * CCU_CPU_AXI_CFG: mux[25:24], divider[3:0]
 * bus_mclk = parents[mux] / (div + 1)
 */
static struct ccu_div cpu_axi_cfg = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_CPU_AXI_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("cpu-axi",
					cpu_parents,
					&ccu_div_ops,
					CLK_SET_RATE_PARENT),
	},
};

/* CCU_AHB2_APB1_DIV: AHB = parent / (div[3:0] + 1) */
static struct ccu_div ahb2_apb1_div = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.common	= {
		.reg		= CCU_AHB2_APB1_DIV,
		.hw.init	= CLK_HW_INIT_PARENTS("ahb2-apb1-div",
					ahb_parents,
					&ccu_div_ops,
					CLK_SET_RATE_PARENT),
	},
};

static SUNXI_CCU_GATE_HW(ahb_clk, "ahb", "ahb2-apb1-div",
			  CCU_AHB2_APB1_DIV, 0, CLK_SET_RATE_PARENT);

static struct ccu_div apb1_clk = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_APB1_DIV_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("apb1",
					apb1_parents,
					&ccu_div_ops,
					CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div apb2_clk = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_APB2_DIV_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("apb2",
					apb2_parents,
					&ccu_div_ops,
					CLK_SET_RATE_PARENT),
	},
};

/* PLL-derived bus clocks (simple dividers) */
static struct ccu_div pll_cpu_axi_div = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.common	= {
		.reg		= CCU_PLL_CPU_AXI_DIV,
		.hw.init	= CLK_HW_INIT("pll-cpu-axi-div",
					"pll-cpu-axi",
					&ccu_div_ops, 0),
	},
};

static struct ccu_div cci_div = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.common	= {
		.reg		= CCU_PLL_CCI_DIV,
		.hw.init	= CLK_HW_INIT("cci-div",
					"pll-cpu-axi",
					&ccu_div_ops, 0),
	},
};

static struct ccu_div dsp_div = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.common	= {
		.reg		= CCU_DSP_CFG,
		.hw.init	= CLK_HW_INIT("dsp-div",
					"pll-cpu-axi",
					&ccu_div_ops, 0),
	},
};

/* ---------------------------------------------------------------------
 * Bus-gate clocks
 *
 * Each bus-gate register uses:
 *   bits [31:24]  – module clock gate (1 = enabled)
 *   bits [23:16]  – bus reset        (1 = de-asserted)
 * We only model the *clock* side here; the reset side is in the
 * reset controller tables below.
 * --------------------------------------------------------------------- */

/* Utility: declare a simple gate inside a bus-BGR register.
 * Each BGR register packs multiple modules – one gate bit each. */

/* ---- UART ---- */
static SUNXI_CCU_GATE_HW(clk_bus_uart0, "bus-uart0", "apb2",
			  CCU_UART_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_uart1, "bus-uart1", "apb2",
			  CCU_UART_BGR, BIT(30), 0);
static SUNXI_CCU_GATE_HW(clk_bus_uart2, "bus-uart2", "apb2",
			  CCU_UART_BGR, BIT(29), 0);
static SUNXI_CCU_GATE_HW(clk_bus_uart3, "bus-uart3", "apb2",
			  CCU_UART_BGR, BIT(28), 0);
static SUNXI_CCU_GATE_HW(clk_bus_uart4, "bus-uart4", "apb2",
			  CCU_UART_BGR, BIT(27), 0);
static SUNXI_CCU_GATE_HW(clk_bus_uart5, "bus-uart5", "apb2",
			  CCU_UART_BGR, BIT(26), 0);

/* ---- SPI ---- */
static SUNXI_CCU_GATE_HW(clk_bus_spi0, "bus-spi0", "apb2",
			  CCU_SPI_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_spi1, "bus-spi1", "apb2",
			  CCU_SPI_BGR, BIT(30), 0);
static SUNXI_CCU_GATE_HW(clk_bus_spi2, "bus-spi2", "apb2",
			  CCU_SPI_BGR, BIT(29), 0);
static SUNXI_CCU_GATE_HW(clk_bus_spi3, "bus-spi3", "apb2",
			  CCU_SPI_BGR, BIT(28), 0);

/* ---- I2C ---- */
static SUNXI_CCU_GATE_HW(clk_bus_i2c0, "bus-i2c0", "apb2",
			  CCU_I2C_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_i2c1, "bus-i2c1", "apb2",
			  CCU_I2C_BGR, BIT(30), 0);
static SUNXI_CCU_GATE_HW(clk_bus_i2c2, "bus-i2c2", "apb2",
			  CCU_I2C_BGR, BIT(29), 0);
static SUNXI_CCU_GATE_HW(clk_bus_i2c3, "bus-i2c3", "apb2",
			  CCU_I2C_BGR, BIT(28), 0);
static SUNXI_CCU_GATE_HW(clk_bus_i2c4, "bus-i2c4", "apb2",
			  CCU_I2C_BGR, BIT(27), 0);
static SUNXI_CCU_GATE_HW(clk_bus_i2c5, "bus-i2c5", "apb2",
			  CCU_I2C_BGR, BIT(26), 0);

/* ---- USB ---- */
static SUNXI_CCU_GATE_HW(clk_bus_usb, "bus-usb", "ahb",
			  CCU_USB_BGR, BIT(31), 0);

/* ---- GMAC ---- */
static SUNXI_CCU_GATE_HW(clk_bus_gmac, "bus-gmac", "ahb",
			  CCU_GMAC_BGR, BIT(31), 0);

/* ---- MMC ---- */
static SUNXI_CCU_GATE_HW(clk_bus_mmc0, "bus-mmc0", "ahb",
			  CCU_MMC_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_mmc1, "bus-mmc1", "ahb",
			  CCU_MMC_BGR, BIT(30), 0);
static SUNXI_CCU_GATE_HW(clk_bus_mmc2, "bus-mmc2", "ahb",
			  CCU_MMC_BGR, BIT(29), 0);

/* ---- CE ---- */
static SUNXI_CCU_GATE_HW(clk_bus_ce, "bus-ce", "ahb",
			  CCU_CE_BGR, BIT(31), 0);

/* ---- DMA ---- */
static SUNXI_CCU_GATE_HW(clk_bus_dma, "bus-dma", "ahb",
			  CCU_DMA_BGR, BIT(31), 0);

/* ---- THERMAL ---- */
static SUNXI_CCU_GATE_HW(clk_bus_thermal, "bus-thermal", "apb2",
			  CCU_THERMAL_BGR, BIT(31), 0);

/* ---- HDMI ---- */
static SUNXI_CCU_GATE_HW(clk_bus_hdmi, "bus-hdmi", "ahb",
			  CCU_HDMI_BGR, BIT(31), 0);

/* ---- DE ---- */
static SUNXI_CCU_GATE_HW(clk_bus_de, "bus-de", "ahb",
			  CCU_DE_BGR, BIT(31), 0);

/* ---- GPU ---- */
static SUNXI_CCU_GATE_HW(clk_bus_gpu, "bus-gpu", "ahb",
			  CCU_GPU_BGR, BIT(31), 0);

/* ---- Audio ---- */
static SUNXI_CCU_GATE_HW(clk_bus_daudio, "bus-daudio", "apb2",
			  CCU_DAUDIO_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_owa, "bus-owa", "apb2",
			  CCU_OWA_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_dmic, "bus-dmic", "apb2",
			  CCU_DMIC_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_audioc, "bus-audioc", "apb2",
			  CCU_AUDIOC_BGR, BIT(31), 0);

/* ---- Misc ---- */
static SUNXI_CCU_GATE_HW(clk_bus_ths, "bus-ths", "apb2",
			  CCU_THS_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_gpadc, "bus-gpadc", "apb2",
			  CCU_GPADC_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_lradc, "bus-lradc", "apb2",
			  CCU_LRADC_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_cec, "bus-cec", "apb2",
			  CCU_CEC_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_pwm, "bus-pwm", "apb2",
			  CCU_PWM_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_spinlock, "bus-spinlock", "ahb",
			  CCU_SPINLOCK_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_drc, "bus-drc", "ahb",
			  CCU_DRC_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_msgbox, "bus-msgbox", "ahb",
			  CCU_MSGBOX_BGR, BIT(31), 0);
static SUNXI_CCU_GATE_HW(clk_bus_rom, "bus-rom", "ahb",
			  CCU_ROM_BGR, BIT(31), 0);

/* ---------------------------------------------------------------------
 * Module (functional) clocks
 * --------------------------------------------------------------------- */

/* ---- UART module clocks: mux[25:24] selects parent ---- */
static struct ccu_mux uart0_clk = {
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_UART0_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("uart0",
				uart_parents, &ccu_mux_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_mux uart1_clk = {
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_UART1_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("uart1",
				uart_parents, &ccu_mux_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_mux uart2_clk = {
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_UART2_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("uart2",
				uart_parents, &ccu_mux_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_mux uart3_clk = {
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_UART3_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("uart3",
				uart_parents, &ccu_mux_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_mux uart4_clk = {
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_UART4_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("uart4",
				uart_parents, &ccu_mux_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_mux uart5_clk = {
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_UART5_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("uart5",
				uart_parents, &ccu_mux_ops,
				CLK_SET_RATE_PARENT),
	},
};

/* ---- SPI module clocks: mux[25:24], divider[3:0] ---- */
static struct ccu_div spi0_mod_clk = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_SPI0_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("spi0-mod",
				spi_parents, &ccu_div_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div spi1_mod_clk = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_SPI1_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("spi1-mod",
				spi_parents, &ccu_div_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div spi2_mod_clk = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_SPI2_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("spi2-mod",
				spi_parents, &ccu_div_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div spi3_mod_clk = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_SPI3_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("spi3-mod",
				spi_parents, &ccu_div_ops,
				CLK_SET_RATE_PARENT),
	},
};

/* ---- I2C module clocks: simple gates (default PLL_PERIPH0_2X) ---- */
static SUNXI_CCU_GATE_HW(i2c0_clk, "i2c0", "pll-periph0-2x",
			  CCU_I2C0_CLK_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(i2c1_clk, "i2c1", "pll-periph0-2x",
			  CCU_I2C1_CLK_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(i2c2_clk, "i2c2", "pll-periph0-2x",
			  CCU_I2C2_CLK_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(i2c3_clk, "i2c3", "pll-periph0-2x",
			  CCU_I2C3_CLK_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(i2c4_clk, "i2c4", "pll-periph0-2x",
			  CCU_I2C4_CLK_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(i2c5_clk, "i2c5", "pll-periph0-2x",
			  CCU_I2C5_CLK_CFG, BIT(31), 0);

/* ---- USB module clocks ---- */
static SUNXI_CCU_GATE_HW(usb_480_clk, "usb-480m", "pll-usb",
			  CCU_USB_CLK_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(usb_12_clk, "usb-12m", "pll-periph0-2x",
			  CCU_USB_CLK_CFG, BIT(29), 0);
static SUNXI_CCU_GATE_HW(usb_ref_clk, "usb-ref", "usb-480m",
			  CCU_USB_REF_CLK, BIT(31), 0);
static SUNXI_CCU_GATE_HW(usb_suspend_clk, "usb-suspend", "osc32k",
			  CCU_USB_SUSPEND_CLK, BIT(31), 0);

/* USB PHYs */
static SUNXI_CCU_GATE_HW(usb_phy0_clk, "usb-phy0", "usb-480m",
			  CCU_USB_PHY_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(usb_phy1_clk, "usb-phy1", "usb-480m",
			  CCU_USB_PHY_CFG, BIT(30), 0);
static SUNXI_CCU_GATE_HW(usb_phy2_clk, "usb-phy2", "usb-480m",
			  CCU_USB_PHY_CFG, BIT(29), 0);

/* ---- MMC module clocks ----
 * CCU_MMCn_CLK_CFG:
 *   mux[25:24]  – clock source
 *   N[15:8]+1   – N divider
 *   Q[7:5]+1    – Q divider
 *   M[3:0]+1    – M divider
 * Effective: clk = source / (N * (Q + 1) * (M + 1))
 */
static struct ccu_div mmc0_mod_clk = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_MMC0_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("mmc0-mod",
				mmc_parents, &ccu_div_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div mmc1_mod_clk = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_MMC1_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("mmc1-mod",
				mmc_parents, &ccu_div_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_div mmc2_mod_clk = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_MMC2_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("mmc2-mod",
				mmc_parents, &ccu_div_ops,
				CLK_SET_RATE_PARENT),
	},
};

/* MMC sub-clocks: sample, detect, bus (gates derived from mod clock) */
static SUNXI_CCU_GATE_HW(mmc0_det, "mmc0-det", "mmc0-mod",
			  CCU_MMC0_CLK_CFG, BIT(27), 0);
static SUNXI_CCU_GATE_HW(mmc0_bus, "mmc0-bus", "mmc0-mod",
			  CCU_MMC0_CLK_CFG, BIT(26), 0);

static SUNXI_CCU_GATE_HW(mmc1_det, "mmc1-det", "mmc1-mod",
			  CCU_MMC1_CLK_CFG, BIT(27), 0);
static SUNXI_CCU_GATE_HW(mmc1_bus, "mmc1-bus", "mmc1-mod",
			  CCU_MMC1_CLK_CFG, BIT(26), 0);

static SUNXI_CCU_GATE_HW(mmc2_det, "mmc2-det", "mmc2-mod",
			  CCU_MMC2_CLK_CFG, BIT(27), 0);
static SUNXI_CCU_GATE_HW(mmc2_bus, "mmc2-bus", "mmc2-mod",
			  CCU_MMC2_CLK_CFG, BIT(26), 0);

/* ---- GMAC module clocks ---- */
static SUNXI_CCU_GATE_HW(gmac_phy_clk, "gmac-phy", "pll-periph0-2x",
			  CCU_GMAC_CLK_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(gmac_ptp_clk, "gmac-ptp", "pll-periph0-2x",
			  CCU_GMAC_CLK_CFG, BIT(30), 0);

/* ---- HDMI ---- */
static struct ccu_div hdmi_mod_clk = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_HDMI_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("hdmi-mod",
				hdmi_parents, &ccu_div_ops,
				CLK_SET_RATE_PARENT),
	},
};

static SUNXI_CCU_GATE_HW(hdmi_ddc_clk, "hdmi-ddc", "osc24m",
			  CCU_HDMI_DDC_CLK, BIT(31), 0);

/* ---- DE (display engine) ---- */
static struct ccu_div de_mod_clk = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_DE_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("de-mod",
				de_parents, &ccu_div_ops,
				CLK_SET_RATE_PARENT),
	},
};

/* ---- GPU ---- */
static struct ccu_div gpu_mod_clk = {
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_GPU_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("gpu-mod",
				gpu_parents, &ccu_div_ops,
				CLK_SET_RATE_PARENT),
	},
};

/* ---- Misc functional clocks ---- */
static SUNXI_CCU_GATE_HW(ths_clk, "ths", "osc24m",
			  CCU_THS_CLK_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(gpadc_clk, "gpadc", "osc24m",
			  CCU_GPADC_CLK_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(lradc_clk, "lradc", "osc24m",
			  CCU_LRADC_CLK_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(cec_clk, "cec", "osc32k",
			  CCU_CEC_CLK_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(pwm_clk, "pwm", "osc24m",
			  CCU_PWM_CLK_CFG, BIT(31), 0);

/* ---- Audio module clocks ---- */
static struct ccu_mux daudio_clk = {
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_DAUDIO_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("daudio",
				audio_parents, &ccu_mux_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_mux owa_clk = {
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_OWA_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("owa",
				audio_parents, &ccu_mux_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_mux dmic_clk = {
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_DMIC_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("dmic",
				audio_parents, &ccu_mux_ops,
				CLK_SET_RATE_PARENT),
	},
};

static struct ccu_mux audioc_clk = {
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= CCU_AUDIOC_CLK_CFG,
		.hw.init	= CLK_HW_INIT_PARENTS("audioc",
				audio_parents, &ccu_mux_ops,
				CLK_SET_RATE_PARENT),
	},
};

static SUNXI_CCU_GATE_HW(spinlock_clk, "spinlock", "osc24m",
			  CCU_SPINLOCK_CLK, BIT(31), 0);
static SUNXI_CCU_GATE_HW(drc_clk, "drc", "osc24m",
			  CCU_DRC_CLK_CFG, BIT(31), 0);
static SUNXI_CCU_GATE_HW(msgbox_clk, "msgbox", "osc24m",
			  CCU_MSGBOX_CLK, BIT(31), 0);
static SUNXI_CCU_GATE_HW(rom_clk, "rom", "osc24m",
			  CCU_ROM_CLK, BIT(31), 0);

/* ---------------------------------------------------------------------
 * Hardware-clock registration table
 *
 * Indices correspond to CLK_* constants from the DT binding header.
 * NULL entries indicate clocks provided by fixed-clock or other nodes.
 * --------------------------------------------------------------------- */

static struct clk_hw_onecell_data sun60i_a733_hw_clks = {
	.num	= CLK_DSP + 1,
	.hws	= {
		/* Root / oscillator clocks (NULL = fixed-clock nodes) */
		[CLK_OSC24M]		= NULL,
		[CLK_OSC32K]		= NULL,
		[CLK_IOSC]		= NULL,

		/* PLLs */
		[CLK_PLL_CPU]		= &pll_cpu_clk.common.hw,
		[CLK_PLL_AUDIO0]	= &pll_audio0_clk.common.hw,
		[CLK_PLL_VIDEO0]	= &pll_video0_clk.common.hw,
		[CLK_PLL_VE]		= &pll_ve_clk.common.hw,
		[CLK_PLL_USB]		= &pll_usb_clk.common.hw,
		[CLK_PLL_CPU_AXI]	= &pll_cpu_axi_clk.common.hw,
		[CLK_PLL_PERIPH0]	= &pll_periph0_clk.common.hw,
		[CLK_PLL_PERIPH1]	= &pll_periph1_clk.common.hw,
		[CLK_PLL_GPU]		= &pll_gpu_clk.common.hw,
		[CLK_PLL_DE]		= &pll_de_clk.common.hw,
		[CLK_PLL_PCIE]		= &pll_pcie_clk.common.hw,
		[CLK_PLL_DDR]		= &pll_ddr_clk.common.hw,

		/* PLL derived clocks */
		[CLK_PLL_AUDIO0_4X]	= &pll_audio0_4x_clk.common.hw,
		[CLK_PLL_AUDIO0_2X]	= &pll_audio0_2x_clk.common.hw,
		[CLK_PLL_AUDIO0_1X]	= &pll_audio0_1x_clk.common.hw,
		[CLK_PLL_VIDEO0_2X]	= &pll_video0_2x_clk.common.hw,
		[CLK_PLL_PERIPH0_2X]	= &pll_periph0_2x_clk.common.hw,
		[CLK_PLL_PERIPH0_4X]	= &pll_periph0_4x_clk.common.hw,

		/* PLL-divided bus clocks */
		[CLK_CPU_AXI]		= &pll_cpu_axi_div.common.hw,
		[CLK_CCI]		= &cci_div.common.hw,
		[CLK_DSP]		= &dsp_div.common.hw,

		/* Bus hierarchy */
		[CLK_AHB]		= &ahb_clk.common.hw,
		[CLK_APB1]		= &apb1_clk.common.hw,
		[CLK_APB2]		= &apb2_clk.common.hw,

		/* Bus gates */
		[CLK_BUS_UART0]		= &clk_bus_uart0.common.hw,
		[CLK_BUS_UART1]		= &clk_bus_uart1.common.hw,
		[CLK_BUS_UART2]		= &clk_bus_uart2.common.hw,
		[CLK_BUS_UART3]		= &clk_bus_uart3.common.hw,
		[CLK_BUS_UART4]		= &clk_bus_uart4.common.hw,
		[CLK_BUS_UART5]		= &clk_bus_uart5.common.hw,
		[CLK_BUS_SPI0]		= &clk_bus_spi0.common.hw,
		[CLK_BUS_SPI1]		= &clk_bus_spi1.common.hw,
		[CLK_BUS_SPI2]		= &clk_bus_spi2.common.hw,
		[CLK_BUS_SPI3]		= &clk_bus_spi3.common.hw,
		[CLK_BUS_I2C0]		= &clk_bus_i2c0.common.hw,
		[CLK_BUS_I2C1]		= &clk_bus_i2c1.common.hw,
		[CLK_BUS_I2C2]		= &clk_bus_i2c2.common.hw,
		[CLK_BUS_I2C3]		= &clk_bus_i2c3.common.hw,
		[CLK_BUS_I2C4]		= &clk_bus_i2c4.common.hw,
		[CLK_BUS_I2C5]		= &clk_bus_i2c5.common.hw,
		[CLK_BUS_USB]		= &clk_bus_usb.common.hw,
		[CLK_BUS_GMAC]		= &clk_bus_gmac.common.hw,
		[CLK_BUS_MMC0]		= &clk_bus_mmc0.common.hw,
		[CLK_BUS_MMC1]		= &clk_bus_mmc1.common.hw,
		[CLK_BUS_MMC2]		= &clk_bus_mmc2.common.hw,
		[CLK_BUS_CE]		= &clk_bus_ce.common.hw,
		[CLK_BUS_DMA]		= &clk_bus_dma.common.hw,
		[CLK_BUS_THERMAL]	= &clk_bus_thermal.common.hw,
		[CLK_BUS_HDMI]		= &clk_bus_hdmi.common.hw,
		[CLK_BUS_DE]		= &clk_bus_de.common.hw,
		[CLK_BUS_GPU]		= &clk_bus_gpu.common.hw,
		[CLK_BUS_DAUDIO]	= &clk_bus_daudio.common.hw,
		[CLK_BUS_OWA]		= &clk_bus_owa.common.hw,
		[CLK_BUS_DMIC]		= &clk_bus_dmic.common.hw,
		[CLK_BUS_AUDIOC]	= &clk_bus_audioc.common.hw,
		[CLK_BUS_THS]		= &clk_bus_ths.common.hw,
		[CLK_BUS_GPADC]		= &clk_bus_gpadc.common.hw,
		[CLK_BUS_LRADC]		= &clk_bus_lradc.common.hw,
		[CLK_BUS_CEC]		= &clk_bus_cec.common.hw,
		[CLK_BUS_PWM]		= &clk_bus_pwm.common.hw,
		[CLK_BUS_SPINLOCK]	= &clk_bus_spinlock.common.hw,
		[CLK_BUS_DRC]		= &clk_bus_drc.common.hw,
		[CLK_BUS_MSGBOX]	= &clk_bus_msgbox.common.hw,
		[CLK_BUS_ROM]		= &clk_bus_rom.common.hw,

		/* Module clocks */
		[CLK_UART0]		= &uart0_clk.common.hw,
		[CLK_UART1]		= &uart1_clk.common.hw,
		[CLK_UART2]		= &uart2_clk.common.hw,
		[CLK_UART3]		= &uart3_clk.common.hw,
		[CLK_UART4]		= &uart4_clk.common.hw,
		[CLK_UART5]		= &uart5_clk.common.hw,
		[CLK_SPI0]		= &spi0_mod_clk.common.hw,
		[CLK_SPI1]		= &spi1_mod_clk.common.hw,
		[CLK_SPI2]		= &spi2_mod_clk.common.hw,
		[CLK_SPI3]		= &spi3_mod_clk.common.hw,
		[CLK_I2C0]		= &i2c0_clk.common.hw,
		[CLK_I2C1]		= &i2c1_clk.common.hw,
		[CLK_I2C2]		= &i2c2_clk.common.hw,
		[CLK_I2C3]		= &i2c3_clk.common.hw,
		[CLK_I2C4]		= &i2c4_clk.common.hw,
		[CLK_I2C5]		= &i2c5_clk.common.hw,
		[CLK_USB_480]		= &usb_480_clk.common.hw,
		[CLK_USB_12]		= &usb_12_clk.common.hw,
		[CLK_USB_PHY]		= &usb_phy0_clk.common.hw,
		[CLK_MMC0]		= &mmc0_mod_clk.common.hw,
		[CLK_MMC0_DET]		= &mmc0_det.common.hw,
		[CLK_MMC0_BUS]		= &mmc0_bus.common.hw,
		[CLK_MMC1]		= &mmc1_mod_clk.common.hw,
		[CLK_MMC1_DET]		= &mmc1_det.common.hw,
		[CLK_MMC1_BUS]		= &mmc1_bus.common.hw,
		[CLK_MMC2]		= &mmc2_mod_clk.common.hw,
		[CLK_MMC2_DET]		= &mmc2_det.common.hw,
		[CLK_MMC2_BUS]		= &mmc2_bus.common.hw,
		[CLK_GMAC]		= &gmac_phy_clk.common.hw,
		[CLK_GMAC_PTP]		= &gmac_ptp_clk.common.hw,
		[CLK_HDMI]		= &hdmi_mod_clk.common.hw,
		[CLK_HDMI_DDC]		= &hdmi_ddc_clk.common.hw,
		[CLK_DE]		= &de_mod_clk.common.hw,
		[CLK_GPU]		= &gpu_mod_clk.common.hw,
		[CLK_THS]		= &ths_clk.common.hw,
		[CLK_GPADC]		= &gpadc_clk.common.hw,
		[CLK_LRADC]		= &lradc_clk.common.hw,
		[CLK_CEC]		= &cec_clk.common.hw,
		[CLK_PWM]		= &pwm_clk.common.hw,
		[CLK_DAUDIO]		= &daudio_clk.common.hw,
		[CLK_OWA]		= &owa_clk.common.hw,
		[CLK_DMIC]		= &dmic_clk.common.hw,
		[CLK_AUDIOC]		= &audioc_clk.common.hw,
		[CLK_SPINLOCK]		= &spinlock_clk.common.hw,
		[CLK_DRC]		= &drc_clk.common.hw,
		[CLK_MSGBOX]		= &msgbox_clk.common.hw,
		[CLK_ROM]		= &rom_clk.common.hw,
	},
};

/* ---------------------------------------------------------------------
 * Reset controller
 *
 * Reset bit positions must match the RST_* constants from the DT
 * binding header.  The sunxi_ccu_reset_init() function walks this
 * table and registers a reset domain for each entry.
 * --------------------------------------------------------------------- */

static struct ccu_reset_map sun60i_a733_ccu_resets[] = {
	[RST_BUS_UART0]		= { CCU_UART_BGR,  BIT(23) },
	[RST_BUS_UART1]		= { CCU_UART_BGR,  BIT(22) },
	[RST_BUS_UART2]		= { CCU_UART_BGR,  BIT(21) },
	[RST_BUS_UART3]		= { CCU_UART_BGR,  BIT(20) },
	[RST_BUS_UART4]		= { CCU_UART_BGR,  BIT(19) },
	[RST_BUS_UART5]		= { CCU_UART_BGR,  BIT(18) },
	[RST_BUS_SPI0]		= { CCU_SPI_BGR,   BIT(23) },
	[RST_BUS_SPI1]		= { CCU_SPI_BGR,   BIT(22) },
	[RST_BUS_SPI2]		= { CCU_SPI_BGR,   BIT(21) },
	[RST_BUS_SPI3]		= { CCU_SPI_BGR,   BIT(20) },
	[RST_BUS_I2C0]		= { CCU_I2C_BGR,   BIT(23) },
	[RST_BUS_I2C1]		= { CCU_I2C_BGR,   BIT(22) },
	[RST_BUS_I2C2]		= { CCU_I2C_BGR,   BIT(21) },
	[RST_BUS_I2C3]		= { CCU_I2C_BGR,   BIT(20) },
	[RST_BUS_I2C4]		= { CCU_I2C_BGR,   BIT(19) },
	[RST_BUS_I2C5]		= { CCU_I2C_BGR,   BIT(18) },
	[RST_BUS_USB]		= { CCU_USB_BGR,   BIT(23) },
	[RST_BUS_GMAC]		= { CCU_GMAC_BGR,  BIT(23) },
	[RST_BUS_MMC0]		= { CCU_MMC_BGR,   BIT(23) },
	[RST_BUS_MMC1]		= { CCU_MMC_BGR,   BIT(22) },
	[RST_BUS_MMC2]		= { CCU_MMC_BGR,   BIT(21) },
	[RST_BUS_CE]		= { CCU_CE_BGR,    BIT(23) },
	[RST_BUS_DMA]		= { CCU_DMA_BGR,   BIT(23) },
	[RST_BUS_THERMAL]	= { CCU_THERMAL_BGR, BIT(23) },
	[RST_BUS_HDMI]		= { CCU_HDMI_BGR,  BIT(23) },
	[RST_BUS_DE]		= { CCU_DE_BGR,    BIT(23) },
	[RST_BUS_GPU]		= { CCU_GPU_BGR,   BIT(23) },
	[RST_BUS_DAUDIO]	= { CCU_DAUDIO_BGR, BIT(23) },
	[RST_BUS_OWA]		= { CCU_OWA_BGR,   BIT(23) },
	[RST_BUS_DMIC]		= { CCU_DMIC_BGR,  BIT(23) },
	[RST_BUS_AUDIOC]	= { CCU_AUDIOC_BGR, BIT(23) },
	[RST_BUS_THS]		= { CCU_THS_BGR,   BIT(23) },
	[RST_BUS_GPADC]		= { CCU_GPADC_BGR, BIT(23) },
	[RST_BUS_LRADC]		= { CCU_LRADC_BGR, BIT(23) },
	[RST_BUS_PWM]		= { CCU_PWM_BGR,   BIT(23) },
	[RST_BUS_SPINLOCK]	= { CCU_SPINLOCK_BGR, BIT(23) },
	[RST_BUS_DRC]		= { CCU_DRC_BGR,   BIT(23) },
	[RST_BUS_MSGBOX]	= { CCU_MSGBOX_BGR, BIT(23) },
};

/* ---------------------------------------------------------------------
 * Platform driver
 * --------------------------------------------------------------------- */

static const struct of_device_id sun60i_a733_ccu_ids[] = {
	{ .compatible = "allwinner,sun60i-a733-ccu" },
	{ }
};

static int sun60i_a733_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	reg = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	/* Register all hardware clocks */
	ret = devm_clk_hw_register_clk_data(&pdev->dev, NULL,
					     sun60i_a733_hw_clks.hws,
					     sun60i_a733_hw_clks.num);
	if (ret) {
		dev_err(&pdev->dev, "failed to register CCU clocks: %d\n",
			ret);
		return ret;
	}

	ret = devm_of_clk_add_hw_provider(&pdev->dev,
					   of_clk_hw_onecell_get,
					   &sun60i_a733_hw_clks);
	if (ret) {
		dev_err(&pdev->dev, "failed to add clock provider: %d\n",
			ret);
		return ret;
	}

	ret = sunxi_ccu_reset_init(pdev, reg, sun60i_a733_ccu_resets,
				   ARRAY_SIZE(sun60i_a733_ccu_resets));
	if (ret) {
		dev_err(&pdev->dev,
			"failed to register reset controller: %d\n", ret);
		return ret;
	}

	return 0;
}

static int sun60i_a733_ccu_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver sun60i_a733_ccu_driver = {
	.probe	= sun60i_a733_ccu_probe,
	.remove	= sun60i_a733_ccu_remove,
	.driver	= {
		.name			= "clk-sun60i-a733",
		.of_match_table		= sun60i_a733_ccu_ids,
	},
};
module_platform_driver(sun60i_a733_ccu_driver);

MODULE_AUTHOR("Allwinner");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Allwinner A733 CCU driver");
