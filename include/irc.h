/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
 
 /**
 * @file
 * @brief Public IRC Driver APIs
 */

#ifndef __IRC_H__
#define __IRC_H__

#include "errno.h"
#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <input_dev.h>

/**
 * @brief IRC Interface
 * @defgroup irc_interface IRC Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif
	
typedef enum {
	IR_FORMAT_9012,
	IR_FORMAT_NEC,
	IR_FORMAT_RC5,
	IR_FORMAT_RC6,
	IR_FORMAT_50462,
	IR_FORMAT_7461,
	IR_FORMAT_50560,
	IR_FORMAT_PULSE_TAB, /* pulse_width seq */
	IR_FORMAT_UNKNOW,
} ir_format_type;

struct ir_input_value {
	uint16_t type;
	uint16_t code;
	uint32_t value;
	uint16_t *pluse_tab;
	uint16_t pluse_tab_len;
};

/**
 * @typedef irc_output_t
 * @brief Callback API upon sending ir key data code
 * See @a irc_output_key() for argument description
 */
typedef int (*irc_output_t)(struct device *dev, struct input_value *val);


/** @brief IRC driver API definition. */
struct irc_driver_api {
	void (*enable)(struct device *dev);
	void (*disable)(struct device *dev);
	void (*register_notify)(struct device *dev, input_notify_t notify);
	void (*unregister_notify)(struct device *dev, input_notify_t notify);
	irc_output_t irc_output;
};


/**
 * @brief Set the key data code for a IRC output.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param key data code.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int irc_output_key(struct device *dev, struct input_value *val)
{
	struct irc_driver_api *api;

	api = (struct irc_driver_api *)dev->driver_api;
	return api->irc_output(dev, val);
}








#ifdef __cplusplus
}
#endif

/**
 * @}
 */


#endif /* __IRC_H__ */

