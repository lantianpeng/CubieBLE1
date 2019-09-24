/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral clock configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_TIMER_H_
#define	_ACTIONS_SOC_TIMER_H_

#include "soc_regs.h"

#define     T0_CTL                                                       (TIMER_REG_BASE+0x000)
#define     T0_VAL                                                       (TIMER_REG_BASE+0x004)
#define     T0_CNT                                                       (TIMER_REG_BASE+0x008)

#define     T1_CTL                                                       (TIMER_REG_BASE+0x040)
#define     T1_VAL                                                       (TIMER_REG_BASE+0x044)
#define     T1_CNT                                                       (TIMER_REG_BASE+0x048)

#ifndef _ASMLANGUAGE



#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_TIMER_H_	*/
