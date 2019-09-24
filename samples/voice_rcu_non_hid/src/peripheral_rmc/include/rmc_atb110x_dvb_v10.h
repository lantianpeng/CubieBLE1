/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define CONFIG_UART_0 1

#define BOARD_PIN_CONFIG	\
	{2, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, \
	{3, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)}, \
	{8, 13}, \
	{22, 13}, \
	{10, 13}, \
	{11, 13}, \
	{1, 2},

#define CONFIG_IRC_TX_PIN 4
#define CONFIG_IRC_RX_PIN 5

#define CONFIG_USE_PWM_LED  1
/* all managed led */
#define LED_PIN_CONFIG	\
			{1, 1, 1}, \
/* app led pin */
#define LED_LPOWER_PIN				1
#define LED_PAIR_PIN					1
#define LED_BTN_PIN						1
#define LED_IR_BTN_PIN				1

/* key reg val mapping to key val */
#define MXKEYPAD_MASK 0x47
#define MXKEYPAD_MAPS \
	{0x00020400, 0x00000000, 0x00000000,  KEY_1}, \
	{0x00004000, 0x00020000, 0x00000000,  KEY_2}, \
	{0x00010004, 0x00000000, 0x00000000,  KEY_5}, \
	{0x00000040, 0x00010000, 0x00000000,  KEY_6}, \
	{0x00000000, 0x00000000, 0x00000200,  KEY_7}, \
	{0x00000000, 0x00000000, 0x00000001,  KEY_8}, \
	{0x00410044, 0x00050000, 0x00000000,  KEY_9}, \
	{0x00014004, 0x00020000, 0x00000000,	KEY_10}

/* key val mapping to standard key */
#define KEY_MAPS \
	{KEY_1,		REMOTE_KEY_VOICE_COMMAND,			0xFF}, \
	{KEY_2,		REMOTE_KEY_MENU,							0xFF}, \
	{KEY_5,		REMOTE_KEY_VOL_INC,						0xFF}, \
	{KEY_6,		REMOTE_KEY_VOL_DEC,						0xFF}, \
	{KEY_7,		REMOTE_KEY_BACK,							0xFF}, \
	{KEY_8,		REMOTE_KEY_POWER,							0xFF}, \
	{KEY_9,		REMOTE_COMB_KEY_OK_BACK,			0xFF},\
	{KEY_10,	REMOTE_COMB_KEY_HCI_MODE,			0xFF},

/* REMOTE_COMB_KEY_OK_BACK --> key5 & key6 */
/* REMOTE_COMB_KEY_HCI_MODE --> key2 & key5 */
#define CUSTOMER_CODE 0x7f80

/* Manufacturer Name */
#define CONFIG_DIS_MANUFACTURER_NAME       "Actions corp."

/* Model Number String */
#define CONFIG_DIS_MODEL                   "ATB110x"

/* PNP ID */
#define CONFIG_DIS_PNP_COMPANY_ID_TYPE    0x01
#define CONFIG_DIS_PNP_VENDOR_ID          0xe0, 0x03
#define CONFIG_DIS_PNP_PRODUCT_ID         0x00, 0x11
#define CONFIG_DIS_PNP_PRODUCT_VERSION    0x00, 0x00
