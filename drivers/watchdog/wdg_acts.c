/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <watchdog.h>
#include "wdg_acts.h"

DEVICE_AND_API_INIT(wdg_acts, CONFIG_WDG_ACTS_DEV_NAME, wdg_acts_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdg_acts_api);

