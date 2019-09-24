/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __APP_INPUT_H__
#define __APP_INPUT_H__

#include <kernel.h>
#include <zephyr/types.h>
#include <misc/printk.h>
#include <gpio.h>

struct input_config {
	u8_t pin;
	u8_t pull_state;      // eg: GPIO_PUD_PULL_UP
	u32_t trigger_mode;   // eg: (GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW)
};

/**
 * @brief input manager init funcion
 *
 * This routine calls init app manager ,called by main
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool input_manager_init(void);

void clear_device_busy(void);

void set_device_busy(void);

#endif
