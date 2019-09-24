/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>

#include "key_hal.h"

#define SYS_LOG_DOMAIN "key"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

struct key_handle  key_handle_entity;
void *key_device_open(key_notify_cb cb, char *dev_name)
{
	struct key_handle *handle = &key_handle_entity;

	handle->input_dev = device_get_binding(dev_name);

	if (!handle->input_dev) {
		SYS_LOG_ERR("cannot found key dev");
		return NULL;
	}

	input_dev_enable(handle->input_dev);

	input_dev_register_notify(handle->input_dev, cb);

	handle->key_notify = cb;

	return handle;
}

void key_device_close(void *handle)
{
	struct key_handle *key = (struct key_handle *)handle;

	input_dev_unregister_notify(key->input_dev, key->key_notify);

	input_dev_disable(key->input_dev);
}
