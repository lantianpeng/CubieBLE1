/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ADCKEY_HAL_H__
#define __ADCKEY_HAL_H__
#include <input_dev.h>

typedef void (*adckey_notify_cb)(struct device *dev, struct input_value *val);

struct adckey_handle {
	char *name;
	struct device *input_dev;
	adckey_notify_cb key_notify;
	bool is_init;
};


/**
 * @brief open adckey device
 *
 * @param callback will be set for notifing keycode from input driver.
 * @param key_device which will be open
 *
 */

struct adckey_handle *adckey_device_open(adckey_notify_cb cb, char *dev_name);

/**
 * @brief close adckey device
 *
 * @param adckey_device handle which will be close
 *
 */

void adckey_device_close(void *handle);

#endif   /* __ADCKEY_HAL_H__ */
