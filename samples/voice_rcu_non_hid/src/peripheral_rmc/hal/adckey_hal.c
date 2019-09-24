/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>

#include "adckey_hal.h"

#define SYS_LOG_DOMAIN "adckey"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

#define CONFIG_ADCKEY_NUM 2

struct adckey_handle adckey_handle_entity[CONFIG_ADCKEY_NUM];

struct adckey_handle *get_adckey_handle(void)
{
	int i;
	for (i = 0; i < CONFIG_ADCKEY_NUM; i++) {
		if (!adckey_handle_entity[i].is_init) {
			return &adckey_handle_entity[i];
		}
	}
	return NULL;
}

struct adckey_handle *adckey_device_open(adckey_notify_cb cb, char *dev_name)
{
	struct adckey_handle *handle = get_adckey_handle();

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

void adckey_device_close(void *handle)
{
	struct adckey_handle *adckey = (struct adckey_handle *)handle;

	input_dev_unregister_notify(adckey->input_dev, adckey->key_notify);

	input_dev_disable(adckey->input_dev);
}
