/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2024 The Linux Foundation. All rights reserved.
 *
 * Allwinner A733 Clock Control Unit (CCU) bindings.
 */

#ifndef _DT_BINDINGS_CLK_SUN60I_A733_CCU_H_
#define _DT_BINDINGS_CLK_SUN60I_A733_CCU_H_

/* Root clocks (from oscillator / PLLs) */
#define CLK_OSC24M		0
#define CLK_OSC32K		1
#define CLK_IOSC		2
#define CLK_PLL_CPU		3
#define CLK_PLL_AUDIO0		4
#define CLK_PLL_VIDEO0		5
#define CLK_PLL_VE		6
#define CLK_PLL_USB		7
#define CLK_PLL_CPU_AXI		8
#define CLK_PLL_PERIPH0		9
#define CLK_PLL_PERIPH1		10
#define CLK_PLL_GPU		11
#define CLK_PLL_DE		12
#define CLK_PLL_PCIE		13
#define CLK_PLL_DDR		14

/* HOSC-gated sources */
#define CLK_HOSC		15
#define CLK_PLL_HOSC		16

/* Audio PLL output divider */
#define CLK_PLL_AUDIO0_4X	17
#define CLK_PLL_AUDIO0_2X	18
#define CLK_PLL_AUDIO0_1X	19

/* Video PLL output divider */
#define CLK_PLL_VIDEO0_4X	20
#define CLK_PLL_VIDEO0_2X	21

/* PERIPH0 fixed dividers */
#define CLK_PLL_PERIPH0_2X	22
#define CLK_PLL_PERIPH0_4X	23

/* Clock source mux + divider: CPU */
#define CLK_CPU			24

/* AHB / APB (bus hierarchy) */
#define CLK_AHB			25
#define CLK_APB1		26
#define CLK_APB2		27

/* Bus gates (enable/disable bus clock to peripherals) */
#define CLK_BUS_UART0		28
#define CLK_BUS_UART1		29
#define CLK_BUS_UART2		30
#define CLK_BUS_UART3		31
#define CLK_BUS_UART4		32
#define CLK_BUS_UART5		33

#define CLK_BUS_SPI0		34
#define CLK_BUS_SPI1		35
#define CLK_BUS_SPI2		36
#define CLK_BUS_SPI3		37

#define CLK_BUS_I2C0		38
#define CLK_BUS_I2C1		39
#define CLK_BUS_I2C2		40
#define CLK_BUS_I2C3		41
#define CLK_BUS_I2C4		42
#define CLK_BUS_I2C5		43

#define CLK_BUS_USB		44
#define CLK_BUS_GMAC		45
#define CLK_BUS_MMC0		46
#define CLK_BUS_MMC1		47
#define CLK_BUS_MMC2		48
#define CLK_BUS_CE		49
#define CLK_BUS_DMA		50
#define CLK_BUS_THERMAL		51
#define CLK_BUS_HDMI		52
#define CLK_BUS_DE		53
#define CLK_BUS_GPU		54
#define CLK_BUS_DAUDIO		55
#define CLK_BUS_OWA		56
#define CLK_BUS_DMIC		57
#define CLK_BUS_AUDIOC		58
#define CLK_BUS_THS		59
#define CLK_BUS_GPADC		60
#define CLK_BUS_LRADC		61
#define CLK_BUS_CEC		62
#define CLK_BUS_PWM		63
#define CLK_BUS_SPINLOCK	64
#define CLK_BUS_DRC		65
#define CLK_BUS_MSGBOX		66
#define CLK_BUS_ROM		67

/* Module (functional) clocks */
#define CLK_UART0		68
#define CLK_UART1		69
#define CLK_UART2		70
#define CLK_UART3		71
#define CLK_UART4		72
#define CLK_UART5		73

#define CLK_SPI0		74
#define CLK_SPI1		75
#define CLK_SPI2		76
#define CLK_SPI3		77

#define CLK_I2C0		78
#define CLK_I2C1		79
#define CLK_I2C2		80
#define CLK_I2C3		81
#define CLK_I2C4		82
#define CLK_I2C5		83

#define CLK_USB_480		84
#define CLK_USB_12		85
#define CLK_USB_PHY		86

#define CLK_MMC0		87
#define CLK_MMC0_SAMPLE		88
#define CLK_MMC0_DET		89
#define CLK_MMC0_BUS		90

#define CLK_MMC1		91
#define CLK_MMC1_SAMPLE		92
#define CLK_MMC1_DET		93
#define CLK_MMC1_BUS		94

#define CLK_MMC2		95
#define CLK_MMC2_SAMPLE		96
#define CLK_MMC2_DET		97
#define CLK_MMC2_BUS		98

#define CLK_GMAC		99
#define CLK_GMAC_TX		100
#define CLK_GMAC_PTP		101

#define CLK_HDMI		102
#define CLK_HDMI_DDC		103

#define CLK_DE			104
#define CLK_DE_DIV		105

#define CLK_GPU			106
#define CLK_GPU_DIV		107

#define CLK_THS			108
#define CLK_GPADC		109
#define CLK_LRADC		110
#define CLK_CEC			111
#define CLK_PWM			112

#define CLK_DAUDIO		113
#define CLK_OWA			114
#define CLK_DMIC		115
#define CLK_AUDIOC		116

/* Vendor alias */
#define CLK_AUDIO		CLK_PLL_AUDIO0_4X

#define CLK_SPINLOCK		117
#define CLK_DRC			118
#define CLK_MSGBOX		119
#define CLK_ROM			120

#define CLK_USB_REF		121
#define CLK_USB_SUSPEND		122

/* /2 clocks */
#define CLK_CPU_AXI		123
#define CLK_CCI			124
#define CLK_DSP			125

/* PLL DDR1 (second DDR PLL) */
#define CLK_PLL_DDR1		126

/* PCIe clocks */
#define CLK_PCIE0_AUX		127
#define CLK_PCIE0_AXI_SLV	128

/* Resets (active-low, bit positions in reset register) */
#define RST_BUS_UART0		0
#define RST_BUS_UART1		1
#define RST_BUS_UART2		2
#define RST_BUS_UART3		3
#define RST_BUS_UART4		4
#define RST_BUS_UART5		5

#define RST_BUS_SPI0		6
#define RST_BUS_SPI1		7
#define RST_BUS_SPI2		8
#define RST_BUS_SPI3		9

#define RST_BUS_I2C0		10
#define RST_BUS_I2C1		11
#define RST_BUS_I2C2		12
#define RST_BUS_I2C3		13
#define RST_BUS_I2C4		14
#define RST_BUS_I2C5		15

#define RST_BUS_USB		16
#define RST_BUS_GMAC		17
#define RST_BUS_MMC0		18
#define RST_BUS_MMC1		19
#define RST_BUS_MMC2		20
#define RST_BUS_CE		21
#define RST_BUS_DMA		22
#define RST_BUS_HDMI		23
#define RST_BUS_DE		24
#define RST_BUS_GPU		25
#define RST_BUS_THS		26
#define RST_BUS_DAUDIO		27
#define RST_BUS_OWA		28
#define RST_BUS_DMIC		29
#define RST_BUS_AUDIOC		30
#define RST_BUS_SPINLOCK	31
#define RST_BUS_DRC		32
#define RST_BUS_MSGBOX		33
#define RST_BUS_GPADC		34
#define RST_BUS_LRADC		35
#define RST_BUS_PWM		36

/* PCIe reset */
#define RST_BUS_PCIE0		37

/* Thermal reset */
#define RST_BUS_THERMAL		38

/*
 * Vendor-name aliases (DTSI uses these names, which map to the
 * canonical indices above).
 */

/* TWI / I2C aliases */
#define CLK_BUS_TWI0		CLK_BUS_I2C0
#define CLK_BUS_TWI1		CLK_BUS_I2C1
#define CLK_BUS_TWI2		CLK_BUS_I2C2
#define CLK_BUS_TWI3		CLK_BUS_I2C3
#define CLK_BUS_TWI4		CLK_BUS_I2C4
#define CLK_BUS_TWI5		CLK_BUS_I2C5

/* SMHC / MMC aliases */
#define CLK_SMHC0		CLK_MMC0
#define CLK_SMHC1		CLK_MMC1
#define CLK_SMHC2		CLK_MMC2

#define CLK_BUS_SMHC0		CLK_BUS_MMC0
#define CLK_BUS_SMHC1		CLK_BUS_MMC1
#define CLK_BUS_SMHC2		CLK_BUS_MMC2

#define RST_BUS_SMHC0		RST_BUS_MMC0
#define RST_BUS_SMHC1		RST_BUS_MMC1
#define RST_BUS_SMHC2		RST_BUS_MMC2

/* TWI / I2C reset aliases */
#define RST_BUS_TWI0		RST_BUS_I2C0
#define RST_BUS_TWI1		RST_BUS_I2C1
#define RST_BUS_TWI2		RST_BUS_I2C2
#define RST_BUS_TWI3		RST_BUS_I2C3
#define RST_BUS_TWI4		RST_BUS_I2C4
#define RST_BUS_TWI5		RST_BUS_I2C5

/* MSGBOX aliases */
#define CLK_MSGBOX0		CLK_MSGBOX
#define RST_BUS_MSGBOX0		RST_BUS_MSGBOX

/* PLL VE alias */
#define CLK_PLL_VE0		CLK_PLL_VE

/* GMAC aliases */
#define CLK_GMAC0		CLK_GMAC
#define RST_BUS_GMAC0		RST_BUS_GMAC

/* GPADC aliases */
#define CLK_GPADC0		CLK_GPADC
#define RST_BUS_GPADC0		RST_BUS_GPADC

/* PWM aliases */
#define CLK_PWM0		CLK_PWM
#define RST_BUS_PWM0		RST_BUS_PWM

/* VE / NPU bus clock placeholders */
#define CLK_BUS_VE_DEC		CLK_BUS_DMA
#define CLK_BUS_VE_ENC		CLK_BUS_DMA
#define CLK_BUS_NPU		CLK_BUS_DMA

/* Timer / LEDC / TWI7-9 bus clocks */
#define CLK_BUS_TIMER		129
#define CLK_TIMER0		130
#define CLK_TIMER1		131
#define CLK_BUS_LEDC		132
#define CLK_LEDC			133
#define CLK_BUS_TWI7		134
#define CLK_BUS_TWI8		135
#define CLK_BUS_TWI9		136

/* USB clocks */
#define CLK_USB0			137
#define CLK_USB0_DEVICE		138
#define CLK_USB0_EHCI		139
#define CLK_USB0_OHCI		140
#define CLK_USB1			141
#define CLK_USB1_EHCI		142
#define CLK_USB1_OHCI		143
#define CLK_USB2			144
#define CLK_USB2_MF		145

/* DMA / MBUS / CE clocks */
#define CLK_DMA			146
#define CLK_MBUS_DMA		147
#define CLK_MBUS_CE		148
#define CLK_CE			149

/* VE (video engine) clocks */
#define CLK_PLL_VE1		150
#define CLK_VE_AHB_GATE		151
#define CLK_VE_DEC			152
#define CLK_VE_DEC_MBUS		153
#define CLK_DEC_MBUS_GATE		154
#define CLK_VE_ENC_AHB_GATE	155
#define CLK_VE_ENC0		156
#define CLK_MBUS_VE_GATE		157
#define CLK_MBUS_VE		158

/* NPU clocks */
#define CLK_NPU			159
#define CLK_PLL_NPU		160

/* PERI0 derived clocks */
#define CLK_PLL_PERI0_800M	161
#define CLK_PLL_PERI0_600M	162
#define CLK_PLL_PERI0_300M	163
#define CLK_PLL_PERI0_200M	164
#define CLK_PLL_PERI0_400M	CLK_PLL_PERIPH0_4X

/* GMAC derived clocks */
#define CLK_GMAC0_MBUS		165
#define CLK_GMAC0_PHY		166

/* GPADC derived clock */
#define CLK_GPADC0_24M		167

/* UART6 */
#define CLK_UART6			168
#define CLK_BUS_UART6			168

/* New reset indices */
#define RST_BUS_TWI7		55
#define RST_BUS_TWI8		56
#define RST_BUS_TWI9		57
#define RST_USB0			58
#define RST_USB0_EHCI		59
#define RST_USB0_OHCI		60
#define RST_USB1			61
#define RST_USB1_EHCI		62
#define RST_USB1_OHCI		63
#define RST_USB2			64
#define RST_BUS_GMAC0_AXI	65
#define RST_BUS_TIMER0		66
#define RST_BUS_LEDC		67
#define RST_BUS_NPU		68
#define RST_BUS_VE_DEC		69
#define RST_BUS_VE_ENC0		70
#define RST_BUS_UART6		71

#endif /* _DT_BINDINGS_CLK_SUN60I_A733_CCU_H_ */
