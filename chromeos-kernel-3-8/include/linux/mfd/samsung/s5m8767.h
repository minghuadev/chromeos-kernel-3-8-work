/*  s5m8767.h
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef __LINUX_MFD_S5M8767_H
#define __LINUX_MFD_S5M8767_H

/* S5M8767 registers */
enum s5m8767_reg {
	S5M8767_REG_ID,
	S5M8767_REG_INT1,
	S5M8767_REG_INT2,
	S5M8767_REG_INT3,
	S5M8767_REG_INT1M,
	S5M8767_REG_INT2M,
	S5M8767_REG_INT3M,
	S5M8767_REG_STATUS1,
	S5M8767_REG_STATUS2,
	S5M8767_REG_STATUS3,
	S5M8767_REG_CTRL1,
	S5M8767_REG_CTRL2,
	S5M8767_REG_LOWBAT1,
	S5M8767_REG_LOWBAT2,
	S5M8767_REG_BUCHG,
	S5M8767_REG_DVSRAMP,
	S5M8767_REG_DVSTIMER2 = 0x10,
	S5M8767_REG_DVSTIMER3,
	S5M8767_REG_DVSTIMER4,
	S5M8767_REG_LDO1,
	S5M8767_REG_LDO2,
	S5M8767_REG_LDO3,
	S5M8767_REG_LDO4,
	S5M8767_REG_LDO5,
	S5M8767_REG_LDO6,
	S5M8767_REG_LDO7,
	S5M8767_REG_LDO8,
	S5M8767_REG_LDO9,
	S5M8767_REG_LDO10,
	S5M8767_REG_LDO11,
	S5M8767_REG_LDO12,
	S5M8767_REG_LDO13,
	S5M8767_REG_LDO14 = 0x20,
	S5M8767_REG_LDO15,
	S5M8767_REG_LDO16,
	S5M8767_REG_LDO17,
	S5M8767_REG_LDO18,
	S5M8767_REG_LDO19,
	S5M8767_REG_LDO20,
	S5M8767_REG_LDO21,
	S5M8767_REG_LDO22,
	S5M8767_REG_LDO23,
	S5M8767_REG_LDO24,
	S5M8767_REG_LDO25,
	S5M8767_REG_LDO26,
	S5M8767_REG_LDO27,
	S5M8767_REG_LDO28,
	S5M8767_REG_UVLO = 0x31,
	S5M8767_REG_BUCK1CTRL1,
	S5M8767_REG_BUCK1CTRL2,
	S5M8767_REG_BUCK2CTRL,
	S5M8767_REG_BUCK2DVS1,
	S5M8767_REG_BUCK2DVS2,
	S5M8767_REG_BUCK2DVS3,
	S5M8767_REG_BUCK2DVS4,
	S5M8767_REG_BUCK2DVS5,
	S5M8767_REG_BUCK2DVS6,
	S5M8767_REG_BUCK2DVS7,
	S5M8767_REG_BUCK2DVS8,
	S5M8767_REG_BUCK3CTRL,
	S5M8767_REG_BUCK3DVS1,
	S5M8767_REG_BUCK3DVS2,
	S5M8767_REG_BUCK3DVS3,
	S5M8767_REG_BUCK3DVS4,
	S5M8767_REG_BUCK3DVS5,
	S5M8767_REG_BUCK3DVS6,
	S5M8767_REG_BUCK3DVS7,
	S5M8767_REG_BUCK3DVS8,
	S5M8767_REG_BUCK4CTRL,
	S5M8767_REG_BUCK4DVS1,
	S5M8767_REG_BUCK4DVS2,
	S5M8767_REG_BUCK4DVS3,
	S5M8767_REG_BUCK4DVS4,
	S5M8767_REG_BUCK4DVS5,
	S5M8767_REG_BUCK4DVS6,
	S5M8767_REG_BUCK4DVS7,
	S5M8767_REG_BUCK4DVS8,
	S5M8767_REG_BUCK5CTRL1,
	S5M8767_REG_BUCK5CTRL2,
	S5M8767_REG_BUCK5CTRL3,
	S5M8767_REG_BUCK5CTRL4,
	S5M8767_REG_BUCK5CTRL5,
	S5M8767_REG_BUCK6CTRL1,
	S5M8767_REG_BUCK6CTRL2,
	S5M8767_REG_BUCK7CTRL1,
	S5M8767_REG_BUCK7CTRL2,
	S5M8767_REG_BUCK8CTRL1,
	S5M8767_REG_BUCK8CTRL2,
	S5M8767_REG_BUCK9CTRL1,
	S5M8767_REG_BUCK9CTRL2,
	S5M8767_REG_LDO1CTRL,
	S5M8767_REG_LDO2_1CTRL,
	S5M8767_REG_LDO2_2CTRL,
	S5M8767_REG_LDO2_3CTRL,
	S5M8767_REG_LDO2_4CTRL,
	S5M8767_REG_LDO3CTRL,
	S5M8767_REG_LDO4CTRL,
	S5M8767_REG_LDO5CTRL,
	S5M8767_REG_LDO6CTRL,
	S5M8767_REG_LDO7CTRL,
	S5M8767_REG_LDO8CTRL,
	S5M8767_REG_LDO9CTRL,
	S5M8767_REG_LDO10CTRL,
	S5M8767_REG_LDO11CTRL,
	S5M8767_REG_LDO12CTRL,
	S5M8767_REG_LDO13CTRL,
	S5M8767_REG_LDO14CTRL,
	S5M8767_REG_LDO15CTRL,
	S5M8767_REG_LDO16CTRL,
	S5M8767_REG_LDO17CTRL,
	S5M8767_REG_LDO18CTRL,
	S5M8767_REG_LDO19CTRL,
	S5M8767_REG_LDO20CTRL,
	S5M8767_REG_LDO21CTRL,
	S5M8767_REG_LDO22CTRL,
	S5M8767_REG_LDO23CTRL,
	S5M8767_REG_LDO24CTRL,
	S5M8767_REG_LDO25CTRL,
	S5M8767_REG_LDO26CTRL,
	S5M8767_REG_LDO27CTRL,
	S5M8767_REG_LDO28CTRL,
};

/* S5M8767 regulator ids */
enum s5m8767_regulators {
	S5M8767_LDO1,
	S5M8767_LDO2,
	S5M8767_LDO3,
	S5M8767_LDO4,
	S5M8767_LDO5,
	S5M8767_LDO6,
	S5M8767_LDO7,
	S5M8767_LDO8,
	S5M8767_LDO9,
	S5M8767_LDO10,
	S5M8767_LDO11,
	S5M8767_LDO12,
	S5M8767_LDO13,
	S5M8767_LDO14,
	S5M8767_LDO15,
	S5M8767_LDO16,
	S5M8767_LDO17,
	S5M8767_LDO18,
	S5M8767_LDO19,
	S5M8767_LDO20,
	S5M8767_LDO21,
	S5M8767_LDO22,
	S5M8767_LDO23,
	S5M8767_LDO24,
	S5M8767_LDO25,
	S5M8767_LDO26,
	S5M8767_LDO27,
	S5M8767_LDO28,
	S5M8767_BUCK1,
	S5M8767_BUCK2,
	S5M8767_BUCK3,
	S5M8767_BUCK4,
	S5M8767_BUCK5,
	S5M8767_BUCK6,
	S5M8767_BUCK7,
	S5M8767_BUCK8,
	S5M8767_BUCK9,
	S5M8767_AP_EN32KHZ,
	S5M8767_CP_EN32KHZ,
	S5M8767_BT_EN32KHZ,

	S5M8767_REG_MAX,
};

#define S5M8767_ENCTRL_SHIFT  6

#define S5M8767_LOW_JITTER_MASK		(1 << 3)

#endif /* __LINUX_MFD_S5M8767_H */
