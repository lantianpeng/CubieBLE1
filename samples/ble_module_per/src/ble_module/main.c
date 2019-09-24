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
#include "app_gpio.h"
#include "app_adc.h"
#include "app_pwm.h"
#include "app_cfg.h"
#include "at_cmd.h"
#include "act_data.h"


/**
 * @brief main module initialization of the application.
 *
 * @details Initialize msg/led/input/sm etc.
 *
 * @return No return value.
 */
__init_once_text void system_app_init(void)
{
	/* init module gpio pin */
	app_gpio_init();
	
	/* init app pwm */
	app_pwm_init();
	
	/* init message manager */
	msg_manager_init();

	/* init at command */
	at_cmd_init();

	/* init act data */
	act_data_init();
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
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	while (1) {
		if (receive_msg(&msg, K_FOREVER)) {
			switch (msg.type) {	
			case MSG_KEY_INPUT:
				break;
			case MSG_BLE_EVENT:
				app_ble_event_handle(msg.cmd, (void*)msg.value);
				break;
			case MSG_AUDIO_INPUT:
				break;
			case MSG_APP_GPIO:
				gpio_event_handle(msg.value);
				break;
			case MSG_OTA_EVENT:
#if CONFIG_OTA_WITH_APP				
				ota_event_handle(msg.value);
#endif
				break;
			case MSG_ADC:
				adc_event_handle(msg.value);
				break;
			case MSG_EXIT_RSSI_MODE:
				app_cfg_event_handle(msg.value);
				break;
			default:
				printk("error message type msg.type %d\n", msg.type);
			break;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}
	}
}
