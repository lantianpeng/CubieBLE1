/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <misc/printk.h>
#include <misc/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/*
 * https://developer.apple.com/ibeacon/
 * weixin: FDA50693-A4E2-4FB1-AFCF-C6EB07647825, Major: 10, Minor: 7
 * apple: E2C56DB5-DFFB-48D2-B060-D0F5A71096E0
 */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),

	BT_DATA_BYTES(0xff,	
		/* Company ID */
		0x4C, 0x00, 

		/* Beacon Type: Proximity Beacons */
		0x02, 0x15,

		/* Proximity UUID */
		/* apple: 0xE2, 0xC5, 0x6D, 0xB5, 0xDF, 0xFB, 0x48,0xD2, 0xB0, 0x60, 0xD0, 0xF5, 0xA7, 0x10, 0x96, 0xE0,*/
		0xFD, 0xA5, 0x06, 0x93, 0xA4, 0xE2, 0x4F, 0xB1, 0xAF, 0xCF, 0xC6, 0xEB, 0x07, 0x64, 0x78, 0x25, 

		/* Major Value (2 Bytes)*/
		0x00, 0x0a,

		/* Minor Value (2 Bytes)*/
		0x00, 0x07,

		/* Measured Power*/
		0xcd 
	) 
 };

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

u8_t bt_ready_flag;
static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	
	bt_ready_flag = 1;

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
				  
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("iBeacon started\n");
}

void app_main(void)
{
	int err;
	
	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
}
