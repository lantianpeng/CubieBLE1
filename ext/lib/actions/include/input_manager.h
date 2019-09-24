/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __INPUT_MANAGER_H__
#define __INPUT_MANAGER_H__
/**
 * @defgroup input_manager_apis App Input Manager APIs
 * @ingroup lib_app_apis
 * @{
 */

/** key action type */
enum KEY_VALUE {
	/** key press down */
	KEY_VALUE_DOWN      = 1,
	/** key press release */
	KEY_VALUE_UP       = 0,
};


/** key press type */
#define			KEY_TYPE_NULL       0x00000000
#define			KEY_TYPE_LONG_DOWN  0x80000000
#define			KEY_TYPE_SHORT_DOWN 0x40000000
#define			KEY_TYPE_DOWN				0x20000000
#define			KEY_TYPE_UP					0x10000000
#define			KEY_TYPE_HOLD       0x08000000
#define			KEY_TYPE_SHORT_UP   0x04000000
#define			KEY_TYPE_LONG_UP    0x02000000
#define			KEY_TYPE_HOLD_UP    0x01000000
#define			KEY_TYPE_ALL        0x3F000000

typedef void (*event_trigger)(u32_t key_value);

typedef bool (*is_need_report_hold_key_t)(u32_t key_value);

/**
 * @brief input manager init funcion
 *
 * This routine calls init input manager ,called by main
 *
 * @param event_cb when keyevent report ,this callback called before
 * key event dispatch.
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

__init_once_text bool input_manager_init(event_trigger event_cb, is_need_report_hold_key_t is_need_report_hold_key);

/**
 * @brief Lock the input event
 *
 * This routine calls lock input event, input event may not send to system
 * if this function called.
 *
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool input_event_lock(void);

/**
 * @brief unLock the input event
 *
 * This routine calls unlock input event, input event may send to system
 * if this function called.
 *
 * @note input_event_lock and input_event_unlock Must match
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool input_event_unlock(void);
/**
 * @brief get the status of input event lock
 *
 *
 * @return true if input event is locked
 * @return false if input event is not locked.
 */
bool input_event_islock(void);

/**
 * @} end defgroup input_manager_apis
 */


#endif
