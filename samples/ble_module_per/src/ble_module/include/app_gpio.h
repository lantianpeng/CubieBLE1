/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __APP_GPIO_H__
#define __APP_GPIO_H__

#include <kernel.h>
#include <zephyr/types.h>
#include <misc/printk.h>
#include <gpio.h>


void gpio_output_wakeup_host(u8_t enable);

void gpio_output_notify_state(u8_t enable);

void gpio_output_flow_ctrl(u8_t enable);

void gpio_event_handle(u32_t event);

/**
 * @brief app gpio init funcion
 *
 * This routine calls init app manager ,called by main
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool app_gpio_init(void);

#endif
