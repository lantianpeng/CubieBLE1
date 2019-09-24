/* main.c - button demo */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/types.h>
#include <misc/printk.h>
#include <zephyr.h>
#include <gpio.h>

/* button */
#define BUTTON_PIN 7

/* trigger low edge */
#define EDGE (GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW)

struct gpio_callback button_cb;

void button_pressed(struct device *gpiob, struct gpio_callback *cb,
		u32_t pins)
{
	printk("pin=%d Button pressed at %d\n",
			find_lsb_set(pins) - 1, k_cycle_get_32());
}

void app_main(void)
{
	struct device *gpio;

	printk("button test\n");

	gpio = device_get_binding(CONFIG_GPIO_ACTS_DRV_NAME);
	if (!gpio) {
		printk("cannot found device gpio\n");
		return;
	}

	gpio_pin_configure(gpio, BUTTON_PIN,
			GPIO_DIR_IN | GPIO_INT | GPIO_PUD_PULL_UP | GPIO_INT_DEBOUNCE | EDGE);
	gpio_init_callback(&button_cb, button_pressed, BIT(BUTTON_PIN));
	gpio_add_callback(gpio, &button_cb);
	gpio_pin_enable_callback(gpio, BUTTON_PIN);
}

