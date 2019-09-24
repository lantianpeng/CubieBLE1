/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SYSTEM_APP_INPUT_H_
#define _SYSTEM_APP_INPUT_H_

/* key code from Univesal Serial Bus HID Usage Tables */
#define REMOTE_USAGE_NONE 				0x0000
#define REMOTE_KEY_POWER					0x0030
#define REMOTE_KEY_PG_DEC					0x0037
#define REMOTE_KEY_PG_INC					0x0038
#define REMOTE_KEY_AIR_MOUSE			0x0039
#define REMOTE_KEY_MENU						0x0040
#define REMOTE_KEY_OK							0x0041
#define REMOTE_KEY_UP 						0x0042
#define REMOTE_KEY_DOWN 					0x0043
#define REMOTE_KEY_LEFT 					0x0044
#define REMOTE_KEY_RIGHT 					0x0045
#define REMOTE_KEY_HELP 					0x0095
#define REMOTE_KEY_PLAY						0x00b0
#define REMOTE_KEY_PAUSE					0x00b1
#define REMOTE_KEY_NEXT_TRACK			0x00b5
#define REMOTE_KEY_PREVIOUS_TRACK 0x00b6
#define REMOTE_KEY_PLAY_PAUSE			0x00cd
#define REMOTE_KEY_MUTE 					0x00e2
#define REMOTE_KEY_VOL_INC				0x00e9
#define REMOTE_KEY_VOL_DEC				0x00ea

#define REMOTE_KEY_SEARCH					0x0221
#define REMOTE_KEY_HOME						0x0223
#define REMOTE_KEY_BACK						0x0224

/* extented consumer key defined by user */

#define REMOTE_KEY_VOICE_COMMAND 				0x020c

#ifdef CONFIG_ANDROID_TV_BOX
#undef REMOTE_KEY_VOICE_COMMAND
#define REMOTE_KEY_VOICE_COMMAND 	REMOTE_KEY_SEARCH
#endif

#define REMOTE_KEY_VOICE_COMMAND_END		0xcccc
#define REMOTE_COMB_KEY_OK_BACK					0xcb00
#define REMOTE_COMB_KEY_TEST_ONE				0xcb01
#define REMOTE_COMB_KEY_TEST_TWO				0xcb02
#define REMOTE_COMB_KEY_TEST_THREE			0xcb03
#define REMOTE_COMB_KEY_TEST_FOUR				0xcb04
#define REMOTE_COMB_KEY_HCI_MODE				0xcb05

/**
 * @brief application input initialization
 *
 */

__init_once_text void system_input_handle_init(void);

/**
 * @brief application input handler
 *
 * @param key event from input subsystem.
 */

void system_input_event_handle(u32_t key_event);

/**
 * @brief send the pending when state of rmc is changed from activing to working
 *
 */

void send_pending_keycode(void);

/**
 * @brief convert input key code into infrared key value
 *
 * @param key code from input subsystem.
 *
 * @return key code of ir.
 */

u16_t acts_get_ir_keycode(int key_code);

/**
 * @brief convert input key code into ble hid key value
 *
 * @param key code from input subsystem.
 *
 * @return key code of hid.
 */

u16_t acts_get_ble_keycode(int key_code);

void IRC_rx_test(void);
void irc_rx_indication(void);
#endif


