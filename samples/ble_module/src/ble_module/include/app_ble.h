/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _APP_BLE_H_
#define _APP_BLE_H_
#include <zephyr/types.h>
#include <kernel.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>

extern u8_t bt_ready_flag;
extern u8_t overlay_ready_flag;

struct app_cb_t {
	struct bt_conn *pconn;
	u8_t work_state;
};

enum BLE_ROLE {
	BLE_ROLE_SLAVE,
	BLE_ROLE_MASTER,
};

enum BLE_STATE {
	INITIALIZED,
	READY,
	PAIRABLE,
	PAIRED,
	INQUIRING,
	ADVERTISING,
	CONNECTING,
	CONNECTED,
	RECONNTING,
	UNKNOWN,
};

#define RSSI_NUM 5
#define DEV_TOTAL 5
#define BLE_NAME_MAX_LEN 18

/* default params */
#define BLE_ADV_NAME_DFLT    "ZRS1000001"
#define BLE_DEVICE_ADDR_DFLT "11:22:33:44:55:66"
#define BLE_TARGET_ADDR_DFLT "00:00:00:00:00:00"
#define BLE_ROLE_DFLT       0
#define BLE_AUTOCONN_DFLT   0

struct ble_dev {
	bt_addr_le_t addr;
	s8_t rssi_cur_index;
	s8_t rssi[RSSI_NUM];
	s8_t rssi_av;
	u8_t rssi_cnt;
	char name[BLE_NAME_MAX_LEN];
};

struct ble_params {
	bool role;
	bool adv_state;
	bool auto_conn;
	u8_t state;
	struct ble_dev dev_list[DEV_TOTAL];
	u8_t dev_cnt;
	bt_addr_le_t targed_addr;
	char adv_name[BLE_NAME_MAX_LEN];
	bt_addr_le_t dev_addr;
};

extern struct ble_params ble_param;

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

int ad_data_set(u8_t type, u8_t *data, u8_t data_len);

int start_scan(struct bt_le_scan_param *scan_param);

void start_discover(void);

void stop_scan(void);

void nofity_enable(void);

void module_ccc_init(void);

void start_adv(void);

struct bt_conn *bt_le_conn_lookup_addr(const bt_addr_le_t *peer);

struct bt_conn *bt_conn_create_le_new(const bt_addr_le_t *peer,
			  const struct bt_le_conn_param *param);

void disconnect(void);

void quick_set_handles(void);

int ble_send_data(u8_t *data, u16_t len);

#endif
