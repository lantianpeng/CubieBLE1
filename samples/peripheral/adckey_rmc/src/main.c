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
#include <soc_pm.h>

#include <input_dev.h>
#include <app_batt.h>

#define CONFIG_ADCKEY_NUM 2

typedef void (*key_notify_cb)(struct device *dev, struct input_value *val);

struct key_handle {
	char *name;
	struct device *input_dev;
	key_notify_cb key_notify;
	bool is_init;
};

struct key_handle key_handle_entity[CONFIG_ADCKEY_NUM];

struct key_handle *get_key_handle(void)
{
	int i;
	for (i = 0; i < CONFIG_ADCKEY_NUM; i++) {
		if (!key_handle_entity[i].is_init) {
			return &key_handle_entity[i];
		}
	}
	return NULL;
}

struct key_handle *key_device_open(key_notify_cb cb, char *dev_name)
{
	struct key_handle *handle = get_key_handle();

	handle->input_dev = device_get_binding(dev_name);

	if (!handle->input_dev) {
		return NULL;
	}

	handle->name = dev_name;

	input_dev_enable(handle->input_dev);

	input_dev_register_notify(handle->input_dev, cb);

	handle->key_notify = cb;

	handle->is_init = true;

	return handle;
}

void key_device_close(void *handle)
{
	struct key_handle *key = (struct key_handle *)handle;

	input_dev_unregister_notify(key->input_dev, key->key_notify);

	input_dev_disable(key->input_dev);
}

void adckey_input_notify(struct device *dev, struct input_value *val)
{
	printk("  code %d, value %d ============\n", val->code, val->value);
}


void app_main(void)
{
	struct key_handle *key;

	app_get_wake_lock();
	k_sleep(500);
	app_release_wake_lock();	
	
	key = key_device_open(adckey_input_notify, CONFIG_INPUT_DEV_ACTS_ADCKEY_0_NAME);
	if (!key) {
		printk("key device %s open failed\n", CONFIG_INPUT_DEV_ACTS_ADCKEY_0_NAME);
	} 

	key = key_device_open(adckey_input_notify, CONFIG_INPUT_DEV_ACTS_ADCKEY_1_NAME);
	if (!key) {
		printk("key device %s open failed\n", CONFIG_INPUT_DEV_ACTS_ADCKEY_1_NAME);
	}

	batt_manager_init();
}
