/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros.
 */

#ifndef _ACTIONS_SOC_H_
#define _ACTIONS_SOC_H_

#include <stdbool.h>
#include "soc_patch.h"

#include "soc_regs.h"
#include "soc_clock.h"
#include "soc_reset.h"
#include "soc_clk_32k.h"
#include "soc_irq.h"
#include "soc_gpio.h"
#include "soc_pinmux.h"
#include "soc_dma.h"
#include "soc_adc.h"
#include "soc_pmu.h"
#include "soc_efuse.h"
#include "soc_memctl.h"
#include "soc_timer.h"
#include "soc_uart.h"
#include "soc_pm.h"
#include "soc_rtc.h"

#ifndef _ASMLANGUAGE

#define SYSTEM_CLOCK_32M (32000000UL)
#define SYSTEM_CLOCK_3M (3000000UL)
#define SYSTEM_CLOCK_32K (32000UL)

#define REBOOT_REASON_ADDR		0x40004038  /* RTC BAK2 & RTC BAK3 */
#define REBOOT_REASON_MAGIC		0x544f4252	/* 'RBOT' */

#define REBOOT_TYPE_GOTO_ADFU 0x1000
#define REBOOT_TYPE_GOTO_DTM  0x1200
#define REBOOT_TYPE_GOTO_OTA	0x1300
#define REBOOT_TYPE_GOTO_APP  0x1400

extern u32_t system_core_clock;

void sys_pm_reboot(int type);
void acts_delay_us(uint32_t us);
void debug_uart_put_char(char data);

#endif /* !_ASMLANGUAGE */



#endif /* _ACTIONS_SOC_H_ */
