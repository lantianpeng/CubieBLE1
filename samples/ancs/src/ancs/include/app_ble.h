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

enum 
{
    /* Initial state */
    app_init = 0,

    /* Fast undirected advertisements configured */
    app_fast_advertising,

    /* Slow undirected advertisements configured */
    app_slow_advertising,

     /* Enters when application is in connected state */
    app_connected,

    /* Enters when disconnect is initiated by the application */
    app_disconnecting,

    /* Idle state */
    app_idle,

    /* Unknown state */
    app_state_unknown

} ;

struct app_cb_t {
	struct bt_conn *pconn;
	bt_addr_le_t bonded_bd_addr;
	bool   remote_gatt_handles_present;
	bool   notif_configuring;
	u8_t   state;
	bool   auth_failure;
	bool   bonded;
	bool   pairing_remove_button_pressed;
	u16_t  temp_handle;
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

void app_ble_event_handle(u8_t event_code, void *event_data);

/**
 * @brief Clear pairing information from non-volatile storage
 *
 * @param none.
 */

void rmc_clear_pair_info(void);
/* This function is used to set the state of the application*/
extern void AppSetState(u8_t new_state);

/* HandleShortButtonPress handles short button presses */
extern void HandleShortButtonPress(void);

/* HandlePairingRemoval clears the cached pairing information */
extern void HandlePairingRemoval(void);

extern void SetTempReadWriteHandle(u16_t handle);

extern u16_t GetTempReadWriteHandle(void);

extern void appConfigureNotifications(struct bt_conn *conn, bool datasource);
#endif
