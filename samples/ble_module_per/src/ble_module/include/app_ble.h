/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _APP_BLE_H_
#define _APP_BLE_H_
#include <zephyr/types.h>

extern u8_t bt_ready_flag;
extern u8_t overlay_ready_flag;

enum {
	ADV_STOP,

	CONN_ADV_START,
	NON_CONN_ADV_START,
};

/* ble event */
enum {
	BLE_EVENT_NULL,
	
	BLE_CONN_OPEN_IND,
	BLE_CONN_SEC_OPEN_IND,
	BLE_CONN_CLOSE_IND,
	
	BLE_DISC_PRIM_SERV_IND,
	BLE_DISC_SERVICE_CHAR_IND,
	BLE_DISC_SERVICE_CHAR_DESC_IND,
	BLE_DISCOVERY_COMPELTE_IND,

};

struct app_cb_t {
	struct bt_conn *pconn;
	u8_t rssi_mode;
	u8_t adv_state;
	u8_t user_force_to_sleep_flag;
};

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
 * @param none.
 */

void rmc_clear_pair_info(void);

/**
 * @brief get rssi of bt_conn
 *
 * @param conn.
 *
 * @return rssi if status is ok.
 * @return 0 if status is fail.
 */
s8_t hci_read_rssi(struct bt_conn *conn);

void clear_ble_state_before_suspend(void);

void set_ble_state_after_resume(void);

void ble_exec_adv_op(u8_t adv_state);

/**
 * @brief ble event handler
 *
 * @param the type of ble event.
 */

void app_ble_event_handle(u8_t event_code, void *event_data);

#endif
