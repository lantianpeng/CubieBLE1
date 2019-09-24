/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for GPIO
 */

#ifndef __GPIO_ACTS_H__
#define __GPIO_ACTS_H__

#include <zephyr/types.h>
#include <gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct acts_gpio_data {
	/* Enabled INT pins generating a cb */
	u32_t cb_pins;
	/* user ISR cb */
	sys_slist_t cb;
};

typedef void (*gpio_config_irq_t)(struct device *dev);
struct acts_gpio_config {
	u32_t base;
	u32_t irq_num;
	gpio_config_irq_t config_func;
};

int gpio_acts_init(struct device *dev);
void gpio_acts_isr(void *arg);
void gpio_acts_config_irq(struct device *dev);

extern const struct gpio_driver_api gpio_acts_drv_api;

#ifdef __cplusplus
}
#endif

#endif	/* __GPIO_ACTS_H__ */
