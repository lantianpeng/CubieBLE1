/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <input_dev.h>

typedef void (*input_notify_t) (struct device *dev, struct input_value *val);

void test_mxkeypad_input_notify(struct device *dev, struct input_value *val)
{
	printk("mxkeypad input value\n");
	printk("  type %d, code %d, value %d\n", val->type, val->code, val->value);
}

void app_main(void)
{
	struct device *mxkeypad;

	printk("Test martrix keypad driver\n");

	mxkeypad = device_get_binding(CONFIG_INPUT_DEV_ACTS_MARTRIX_KEYPAD_NAME);
	if (!mxkeypad)
		printk("Cannot found martrix keypad device: %s\n", CONFIG_INPUT_DEV_ACTS_MARTRIX_KEYPAD_NAME);

	input_dev_enable(mxkeypad);

	input_dev_register_notify(mxkeypad, test_mxkeypad_input_notify);

	printk("Wait for martrix keypad key pressed\n");

#if 0
	k_sleep(10000);

	input_dev_unregister_notify(mxkeypad, test_mxkeypad_input_notify);

	input_dev_disable(mxkeypad);
#endif
	while (1)
		;
}
