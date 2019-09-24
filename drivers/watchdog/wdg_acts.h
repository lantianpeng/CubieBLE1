/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for watchdog
 */

#ifndef __WDG_ACTS_H__
#define __WDG_ACTS_H__

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct wdt_driver_api wdg_acts_api;
int wdg_acts_init(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif	/* __WDG_ACTS_H__ */
