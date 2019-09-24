/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <drivers/system_timer.h>

#define CONFIG_SYSTEM_CLOCK_INIT_PRIORITY 0

SYS_DEVICE_DEFINE("sys_clock", _sys_clock_driver_init, device_pm_control_nop,
		PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
