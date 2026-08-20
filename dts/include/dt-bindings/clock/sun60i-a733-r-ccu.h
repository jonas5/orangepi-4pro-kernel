/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2024 The Linux Foundation. All rights reserved.
 *
 * Allwinner A733 R-CCU (PRCM / Resource Clock Control Unit) bindings.
 */

#ifndef _DT_BINDINGS_CLK_SUN60I_A733_R_CCU_H_
#define _DT_BINDINGS_CLK_SUN60I_A733_R_CCU_H_

/* R-CCU / PRCM clock IDs (logical; used in DTS and driver) */

#define R_CLK_OSC32K		0
#define R_CLK_IOSC		1
#define R_CLK_PLL_AUDIO0	2

/* RTC domain */
#define R_CLK_RTC_32K		3
#define R_CLK_RTC_1K		4
#define R_CLK_CEC_32K		5

/* Reset- / low-power-domain bus clocks */
#define R_CLK_R_PIO		6
#define R_CLK_R_APB1		7

/* Peripheral clocks in the R domain */
#define R_CLK_R_UART		8
#define R_CLK_R_I2C		9
#define R_CLK_R_PWM		10
#define R_CLK_R_SPINLOCK	11
#define R_CLK_R_THERMAL		12
#define R_CLK_R_SID		13
#define R_CLK_R_CPUCFG		14

/* Module clocks inside R domain */
#define R_CLK_R_UART_CK		15
#define R_CLK_R_I2C_CK		16
#define R_CLK_THERMAL_SENSOR	17
#define R_CLK_R_SID_CK		18

/* Fixed dividers */
#define R_CLK_R_AHB		19
#define R_CLK_R_APB1_DIV	20

/* R-PPU clocks */
#define R_CLK_R_PPU		21

/* R-CCU resets */
#define R_RST_RTC		0
#define R_RST_R_PIO		1
#define R_RST_R_UART		2
#define R_RST_R_I2C		3
#define R_RST_R_PWM		4
#define R_RST_R_SPINLOCK	5
#define R_RST_R_THERMAL		6
#define R_RST_R_SID		7
#define R_RST_R_CPUCFG		8

/* R-PPU resets */
#define R_RST_R_PPU0		9
#define R_RST_R_PPU1		10

/*
 * Vendor-name aliases (DTSI uses these names, which map to the
 * canonical indices above).
 */
#define CLK_R_AHB		R_CLK_R_AHB
#define CLK_R_APB0		R_CLK_R_APB1
#define CLK_R_TWI0		R_CLK_R_I2C
#define RST_BUS_R_TWI0		R_RST_R_I2C
#define RST_BUS_RTC		R_RST_RTC
#define CLK_R_PPU		R_CLK_R_PPU
#define RST_BUS_R_PPU0		R_RST_R_PPU0
#define RST_BUS_R_PPU1		R_RST_R_PPU1

/* New clock defines */
#define CLK_RTC_1K		22
#define CLK_RTC_SPI		23

#endif /* _DT_BINDINGS_CLK_SUN60I_A733_R_CCU_H_ */
