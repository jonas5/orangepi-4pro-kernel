/* SPDX-License-Identifier: (GPL-2.0-or-later OR MIT) */
/*
 * Device Tree binding constants for Allwinner A733 GPIO controller
 */

#ifndef _DT_BINDINGS_GPIO_SUN60I_A733_H
#define _DT_BINDINGS_GPIO_SUN60I_A733_H

#define GPIO_BANK_A	0
#define GPIO_BANK_B	1
#define GPIO_BANK_C	2
#define GPIO_BANK_D	3
#define GPIO_BANK_E	4
#define GPIO_BANK_F	5
#define GPIO_BANK_G	6
#define GPIO_BANK_H	7
#define GPIO_BANK_I	8
#define GPIO_BANK_J	9
#define GPIO_BANK_K	10
#define GPIO_BANK_L	11
#define GPIO_BANK_M	12

#define GPIO_BANK(letter, offset)	(GPIO_BANK_ ## letter + (offset))

#endif /* _DT_BINDINGS_GPIO_SUN60I_A733_H */
