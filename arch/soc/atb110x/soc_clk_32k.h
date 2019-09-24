/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file 32k clk configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_CLK_32K_H_
#define	_ACTIONS_SOC_CLK_32K_H_

#include "soc_regs.h"

#define		BLE_32K_CTL								(CMU_DIGITAL_BASE+0x0000)
#define		BLE_32K_CTL_CKO32K_OEN			2
#define		BLE_32K_CTL_XTAL32K_ON			1
#define		BLE_32K_CTL_XTAL32K_REQ		0

/*--------------REGISTER ADDRESS----------------------*/
#define		HCL32K_CTL										(HCL_REG_BASE+0x0000)
#define		HCL32K_DATA										(HCL_REG_BASE+0x0004)

/*--------------BITS LOCATION--------------------------*/
#define		HCL32K_CTL_HCL_UPDATA								13
#define		HCL32K_CTL_HCL_MS										12
#define		HCL32K_CTL_CAL_DELAY_TIME_BPS				11
#define		HCL32K_CTL_HCL_INTERVAL_E						10
#define		HCL32K_CTL_HCL_INTERVAL_SHIFT				8
#define		HCL32K_CTL_HCL_INTERVAL_MASK				(0x7<<8)
#define		HCL32K_CTL_HOSC_OSC_TIME_E					7
#define		HCL32K_CTL_HOSC_OSC_TIME_SHIFT			6
#define		HCL32K_CTL_HOSC_OSC_TIME_MASK				(0x3<<6)
#define		HCL32K_CTL_CAL_DELAY_TIME_E					5
#define		HCL32K_CTL_CAL_DELAY_TIME_SHIFT			4
#define		HCL32K_CTL_CAL_DELAY_TIME_MASK			(0x3<<4)
#define		HCL32K_CTL_HCL_32K_EN								0

#define		HCL32K_DATA_DATA_E									16
#define		HCL32K_DATA_DATA_SHIFT							0
#define		HCL32K_DATA_DATA_MASK								(0x1FFFF<<0)


#ifndef _ASMLANGUAGE

typedef enum {
	HCL_NORMAL = 0x0,
	HCL_AUTO = 0x1,
} hcl_mode_t;

void acts_set_hcl_32k(hcl_mode_t mode);
void acts_set_xtal_32k(void);
void acts_wait_xtal_32k_on(void);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_CLK_32K_H_	*/
