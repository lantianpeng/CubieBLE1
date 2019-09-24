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
#include <input_dev.h>

#include "hci_core.h"
#include "hci_core_patch.h"

#include <dis.h>
#include <bas.h>
#include <hog.h>
#include <wp.h>
#include "msg_manager.h"
#include "system_app_sm.h"
#include "system_app_input.h"
#include "system_app_audio.h"
#include "system_app_timer.h"
#include "system_app_led.h"
#include "ota.h"
#include "soc.h"
#include "soc_pm.h"
#include "system_app_batt.h"
#include "system_app_audio.h"
#include "system_app_ble.h"
#include "system_app_storage.h"
#define SYS_LOG_DOMAIN "main"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

/**
 * @brief main module initialization of the application.
 *
 * @details Initialize msg/led/input/sm etc.
 *
 * @return No return value.
 */
__init_once_text void system_app_init(void)
{
	/* init message manager */
	msg_manager_init();

	/*led manager initialization */
	led_manager_init();

	/* input manager initialization */
	system_input_handle_init();

	/* led handle initialization */
	system_led_handle_init();

	/* state machine of rmc initialization */
	rmc_sm_init();

	/* app timer of rmc initialization */
	rmc_timer_init();

	/* audio module initialization */
	audio_init();

	/* batt manager initialization */
	batt_manager_init();
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
	int result = 0;

	/* main module initialization */
	system_app_init();

	/* Initialize the Bluetooth Subsystem */
	app_get_wake_lock();
	err = bt_enable(bt_ready);
	if (err) {
		app_release_wake_lock();
		SYS_LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
  /* init storage */
  #ifdef	CONFIG_AUDIO_OUT
	system_app_storage_init();
	reset_stroage();
	#endif
	
	/*******IRC RX********/
	#ifdef CONFIG_IRC_RX
	IRC_rx_test();
	#endif
	
	/* start the timer of No action */
	rmc_timer_start(ID_NO_ACT_TIMEOUT);

	/* enter into message processing main loop */
	while (1) {
		if (receive_msg(&msg, K_FOREVER)) {
			switch (msg.type) {
			case MSG_KEY_INPUT:
				SYS_LOG_DBG("MSG_KEY_INPUT");
				rmc_timer_start(ID_NO_ACT_TIMEOUT);
				if (bt_ready_flag)
					system_input_event_handle(msg.value);
				break;
			case MSG_BLE_STATE:
				SYS_LOG_INF("MSG_BLE_STATE");
				system_ble_event_handle(msg.value);
				break;
			case MSG_AUDIO_INPUT:
				rmc_timer_start(ID_NO_ACT_TIMEOUT);
				system_audio_event_handle(msg.ptr);
				break;
			case MSG_APP_TIMER:
				SYS_LOG_INF("MSG_APP_TIMER %d", msg.value);
				system_app_timer_event_handle(msg.value);
				break;
			case MSG_OTA_EVENT:
				rmc_timer_start(ID_NO_ACT_TIMEOUT);
				ota_event_handle(msg.value);
				break;
			case MSG_LOW_POWER:
				rmc_sm_execute(RMC_MSG_LOW_POWER);
				break;
			default:
				SYS_LOG_ERR("error message type msg.type %d", msg.type);
			break;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}
	}
}
