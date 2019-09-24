/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IRC_HAL_H__
#define __IRC_HAL_H__

#include <input_dev.h>
#include <device.h>

struct irc_handle {
	struct device *irc_dev;
#ifdef CONFIG_IRC_SW_RX
	struct device *sw_irc_dev;
	input_notify_t event_cb;
	struct k_timer learn_mode_timer;
	u32_t cur_learn_keycode;
	u8_t cur_learn_state;
	u8_t has_set_learn_key;
	u8_t sw_ir_learn_mode;
#endif
	u8_t state;
};

struct nv_key_map {
	u16_t learn_key_code;
	char *nv_key_name;
	u16_t def_code;
};

typedef enum {
	IR_LEARN_SUCCESS,
	IR_LEARN_FAILED,
	IR_LEARN_TIMEOUT,
	IR_LEARN_READY,
} ir_learn_state;

/**
 * @brief open irc device
 *
 * @param irc device name which will be open
 *
 */
void *irc_device_open(char *dev_name);

/**
 * @brief enable irc rx channel
 *
 * @param irc device handle
 * @param input_notify_t be used for notify irc rx channel keycode
 *
 */

void irc_device_enable_rx(void *handle, input_notify_t cb);

/**
 * @brief send irc keycode using tx channel
 *
 * @param irc device handle
 * @param irc key code 
 * @param key_state which is key_down or key_up
 *
 */

void irc_device_send_key(void *handle, u32_t key_code, u8_t key_state);

/**
 * @brief send irc keycode using tx channel
 *
 * @param irc device handle
 * @param irc key code 
 * @param key_state which is key_down or key_up
 *
 */

void irc_device_send_learn_key(void *handle, u32_t key_code, u8_t key_state);

void irc_device_exit_learn_mode(void *handle);

/**
 * @brief disable irc rx channel
 *
 * @param irc device handle
 *
 */

void irc_device_disable_rx(void *handle);
/**
 * @brief enable irc rx channel
 *
 * @param irc device handle
 *
 * @return state (key down or key up)
 */

u8_t irc_device_state_get(void *handle);

/**
 * @brief close irc device
 *
 * @param irc device handle which will be close
 *
 */

void irc_device_close(void *handle);

#endif   /* __IRC_HAL_H__ */
