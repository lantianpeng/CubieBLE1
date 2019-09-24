/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define CONFIG_UART_0 1

/* input-adckey */
#define CONFIG_INPUT_DEV_ACTS_ADCKEY_0_NAME "ADCKEY_0"

#define BOARD_PIN_CONFIG	\
	{2, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},\
	{3, 3 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3)},\
	{11, 13},\
	{29, 13},\
	{10, 2},\

//#define CONFIG_IRC_SW_RX
#define CONFIG_IRC_TX_PIN 21
#define CONFIG_IRC_RX_PIN 20
	
#define LED_IRC_RX_PIN    6
#define LED_BLE_PIN       10
#define LED_IR_TX_PIN     7

#define CONFIG_USE_PWM_LED  1
//#define CONFIG_USE_GPIO_LED 1

#if CONFIG_USE_PWM_LED
/* all managed led */
#define LED_PIN_CONFIG	\
			{LED_BLE_PIN,    0, 0}, 
#else
			
/* all managed led */
#define LED_PIN_CONFIG \
			{LED_BLE_PIN, 0xFF, 1}, \
			
#endif

			
/* app led pin */
#define LED_LPOWER_PIN		  LED_BLE_PIN
#define LED_PAIR_PIN		    LED_BLE_PIN
#define LED_BTN_PIN			    LED_BLE_PIN
#define LED_BLE_BTN_PIN		  LED_BLE_PIN  
#define LED_IR_BTN_PIN		  LED_BLE_PIN

			
/* key reg val mapping to key val */
#define MXKEYPAD_MASK 0x84
#define MXKEYPAD_MAPS \
	{0x00000000, 0x00000000, 0x00000080,  KEY_1}, \
	{0x00000000, 0x00000000, 0x00000004,  KEY_2}, \
	{0x00000000, 0x00000000, 0x00000400,  KEY_3}, \
	{0x00000000, 0x00000000, 0x00008000,  KEY_4}, \
	{0x00000000, 0x00000000, 0x00008004,  KEY_12}, \
	{0x00000000, 0x00000000, 0x00008400,  KEY_13}, \
	

/*IR key value*/
#define IR_KB_POWER                0x18
#define IR_KB_MUTE                 0x5B
#define IR_KB_MENU                 0x4e
#define IR_KB_UP                   0x46
#define IR_KB_DOWN                 0x16
#define IR_KB_LEFT                 0x47
#define IR_KB_RIGHT                0x15
#define IR_KB_ENTER                0x55
#define IR_KB_ACBACK               0x40
#define IR_KB_ACHOME               0x4f
#define IR_KB_VOL_INC            	 0x14
#define IR_KB_VOL_DEC            	 0x10
#define IR_CUSTOMER_CODE           0xff00  

/* use bat val 3v as default */
#define ADCKEY_0_MAPS \
	{28,     0,    KEY_5},   /* (3/30)*1024/3.6 = 28.4       */ \
	{60,    123,   KEY_6},   /* (7/33)*1024/3.6 =  60.3  */ \
	{95,    240,   KEY_7},   /* (12/36)*1024/3.6 = 94.8  */ \
	{126,   372,   KEY_8},   /* (19/43)*1024/3.6 = 125.6 */ \
	{156,   507,   KEY_9},   /* (29/53)*1024/3.6 = 155.6 */ \
	{192,   633,   KEY_10},  /* (49/73)*1024/3.6 =  191.6*/ \
	{220,   763,   KEY_11},  /* (82/106)*1024/3.6 =  220.0*/

	
/* key val mapping to standard key */
#define KEY_MAPS \
	{KEY_1, 		REMOTE_KEY_VOICE_COMMAND,   IR_KB_MENU },\
	{KEY_2,		  REMOTE_KEY_VOL_INC,   		  IR_KB_VOL_INC },\
	{KEY_3, 		REMOTE_KEY_VOL_DEC,    	    IR_KB_VOL_DEC },\
	{KEY_4, 		REMOTE_KEY_POWER,  	        IR_KB_POWER },\
	{KEY_5,		  REMOTE_KEY_RIGHT,   		    IR_KB_RIGHT },\
	{KEY_6,		  REMOTE_KEY_DOWN,   		      IR_KB_DOWN },\
	{KEY_7,		  REMOTE_KEY_OK,   		        IR_KB_ENTER },\
	{KEY_8,		  REMOTE_KEY_LEFT,   	        IR_KB_LEFT },\
	{KEY_9,		  REMOTE_KEY_UP,   	          IR_KB_UP },\
	{KEY_10,		REMOTE_KEY_BACK,   		      IR_KB_ACBACK },\
	{KEY_11,		REMOTE_KEY_HOME,   		      IR_KB_ACHOME },\
  {KEY_12,		REMOTE_COMB_KEY_OK_BACK,		KEY_RESERVED},\
	{KEY_13,    REMOTE_COMB_KEY_HCI_MODE,   KEY_RESERVED},\
	
	
/*
power,mute,TV, VOD, 0c
*/

#define CUSTOMER_CODE IR_CUSTOMER_CODE

#undef CONFIG_BT_DEVICE_NAME
#define CONFIG_BT_DEVICE_NAME "CubieBLE1"

/* Manufacturer Name */
#define CONFIG_DIS_MANUFACTURER_NAME       "PT corp."

/* Model Number String */
#define CONFIG_DIS_MODEL                   "ATB110x"

/* PNP ID */
#define CONFIG_DIS_PNP_COMPANY_ID_TYPE    0x02
#define CONFIG_DIS_PNP_VENDOR_ID          0x54, 0x2B
#define CONFIG_DIS_PNP_PRODUCT_ID         0x00, 0x16
#define CONFIG_DIS_PNP_PRODUCT_VERSION    0x00, 0x00
