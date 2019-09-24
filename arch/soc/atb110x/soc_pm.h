/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file pmu configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_PM_H_
#define	_ACTIONS_SOC_PM_H_

#include "soc_regs.h"

#ifndef _ASMLANGUAGE

extern int (*p_sys_soc_suspend)(s32_t);

extern u32_t Image$$RAM_RETENTION$$Base[];
extern u32_t Image$$RAM_RETENTION$$Limit[];

extern u32_t application_wake_lock;

void app_get_wake_lock(void);
void app_release_wake_lock(void);

int _sys_soc_suspend(s32_t ticks);

void idle(void *unused1, void *unused2, void *unused3);

void _timer_deepsleep_enter(s32_t ticks);
void _timer_deepsleep_exit(void);


#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_PM_H_	*/
