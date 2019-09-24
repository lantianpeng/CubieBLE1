/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __KEY_HAL_H__
#define __KEY_HAL_H__
#include <input_dev.h>

typedef void (*key_notify_cb)(struct device *dev, struct input_value *val);

struct key_handle {
	struct device *input_dev;
	key_notify_cb key_notify;
};

/**
 * @brief open key device
 *
 * @param callback will be set for notifing keycode from input driver.
 * @param key_device which will be open
 *
 */

void *key_device_open(key_notify_cb cb, char *dev_name);

/**
 * @brief close key device
 *
 * @param key_device handle which will be close
 *
 */

void key_device_close(void *handle);

#endif   /* __ADCKEY_HAL_H__ */
