/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file efuse configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_EFUSE_H_
#define	_ACTIONS_SOC_EFUSE_H_

#define     EFUSE_BASE                                                        0x40008000
#define     PMU_DEBUG                                                         (EFUSE_BASE+0x7C)

#define     PMU_DEBUG_DBGPWR_WK_EN                                            2
#define     PMU_DEBUG_PMU_DEBUG_SEL_E                                         1
#define     PMU_DEBUG_PMU_DEBUG_SEL_SHIFT                                     0
#define     PMU_DEBUG_PMU_DEBUG_SEL_MASK                                      (0x3<<0)

#ifndef _ASMLANGUAGE

uint32_t acts_efuse_read(uint8_t offset);
uint32_t acts_efuse_read_val(uint8_t offset);
void acts_set_vd12_before_efuse_read(void);
void acts_set_vd12_after_efuse_read(void);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_EFUSE_H_	*/
