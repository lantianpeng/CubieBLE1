/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SYSTEM_APP_BLE_H_
#define _SYSTEM_APP_BLE_H_

extern u8_t bt_ready_flag;
extern u8_t overlay_ready_flag;
extern bool sc_supported;
extern u8_t pair_comb_key_timer_start;

/**
 * @brief Callback for notifying that Bluetooth has been enabled.
 *
 *  A function will be called by the bt_enable() function
 * @param err zero on success or (negative) error code otherwise.
 */

__init_once_text void bt_ready(int err);

/**
 * @brief ble event handler
 *
 * @param the type of ble event.
 */

void system_ble_event_handle(u32_t ble_event);

/**
 * @brief function of sending voice data event
 *
 * @param the buffer which contain audio data.
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

int hid_app_voice_report_event(u8_t *buffer);

/**
 * @brief function of sending remote key event
 *
 * @param remote key code.
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

int hid_app_remote_report_event(u16_t button);

/**
 * @brief Clear pairing information from non-volatile storage
 *
 */

void rmc_clear_pair_info(void);

/**
 * @brief check if ble is security ready
 *
 * @return true if security is ready.
 * @return false if security is not ready.
 */
bool rmc_security_ready(void);


bool is_paired(void);

#endif
