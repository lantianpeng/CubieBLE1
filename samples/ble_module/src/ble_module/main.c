/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <misc/printk.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/storage.h>
#include "keys.h"
#include "hci_core.h"
#include "hci_core_patch.h"
#include <dis.h>
#include <bas.h>
#include <wp.h>
#include "msg_manager.h"
#include "ota.h"
#include "soc.h"
#include "soc_pm.h"
#include "app_ble.h"
#include "app_input.h"
#include "at_cmd.h"
#include "act_data.h"
#include "uart_pipe.h"
#include <led_manager.h>
#include "misc.h"

extern struct app_cb_t app_cb;
extern void leds_status_reset(void);

/**
 * @brief main module initialization of the application.
 *
 * @details Initialize msg/led/input/at etc.
 *
 * @return No return value.
 */
__init_once_text void system_app_init(void)
{
	/* init led manager */
	led_manager_init();
	
	/* init input manager */
	input_manager_init();
	
	/* init message manager */
	msg_manager_init();

	/* uart pipe init */
	uart_pipe_init();	
	
	/* at command init */
	at_cmd_init();

	/* act data init */
	act_data_init();
	
	/* ccc param init */
	module_ccc_init();
	
	/* reset led status */
	leds_status_reset();
}

/**
 * @brief the entry function of the application.
 *
 * @details Initialize app module and enter into msg mainloop.
 *
 * @return No return value. 
 */
void app_main(void)
{
	int err;
	struct app_msg msg = {0};
	
	app_get_wake_lock();
	
	/* main module initialization */
	system_app_init();
		
	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		app_release_wake_lock();
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	while (1) {
		if (receive_msg(&msg, K_FOREVER)) {
			switch (msg.type) {
			case MSG_CONNECTED:
#if CONFIG_QUICK_CONNECT
				quick_set_handles();		
				send_async_msg(MSG_DISCOVER_COMPLETE);
#else
				start_discover();
#endif
				break;				
			case MSG_DISCOVER_COMPLETE:
				nofity_enable();
				break;
			case MSG_SCAN_TIME_OUT:
				stop_scan();
				at_cmd_response_ok();
				if (ble_param.auto_conn) {
					extern int get_max_riss_dev_index(void);
					extern int at_cmd_conn_set(char *p_para);
					int index = get_max_riss_dev_index();
					if (index >= 0) {
						char index_str = index + '1';
						at_cmd_conn_set(&index_str);
					} else {
						printk("No device found\n");
					}
				}
				break;
			case MSG_CREATE_LE_TIME_OUT:
			{
				extern void create_le_timeout(void);
				create_le_timeout();
				break;
			}
#if CONFIG_AUTO_RECONNECT
			case MSG_RECONNECTING:
			{
				extern struct bt_le_scan_param app_scan_cfg;
				start_scan(&app_scan_cfg);
				ble_state_update(RECONNTING);
				break;
			}
#endif			
			case MSG_DISCONNECT:
				disconnect();			
				break;
			case MSG_DISCONNECTED:
				if (ble_param.role == BLE_ROLE_SLAVE) {
					start_adv();
				}
				break;
			case MSG_OTA_EVENT:
#if CONFIG_OTA_WITH_APP				
				ota_event_handle(msg.value);
#endif
				break;
			default:
				printk("error message type msg.type %d\n", msg.type);
			break;
			}
		}
	}
}
