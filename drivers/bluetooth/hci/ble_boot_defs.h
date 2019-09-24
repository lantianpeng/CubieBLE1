/* ble_boot_defs.h - llcc based Bluetooth driver */

/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_BOOT_DEFS_H
#define BLE_BOOT_DEFS_H

/**************************************************************************************************
  Macros
**************************************************************************************************/

/* boot vendor specific OCF range is 0x000-0x00F. */
#define BLE_BOOT_OCF_BASE                0x000
#define BLE_BOOT_OCF_END                 0x00F

#define BLE_BOOT_OPCODE_TRIM_LOAD        HCI_OPCODE(HCI_OGF_VENDOR_SPEC, BLE_BOOT_OCF_BASE + 0x03)   /*!< Trim load command.  */
#define BLE_BOOT_OPCODE_TRIM_DATA        HCI_OPCODE(HCI_OGF_VENDOR_SPEC, BLE_BOOT_OCF_BASE + 0x04)   /*!< Trim data command. */

#define BLE_BOOT_LEN_TRIM_LOAD           4
#define BLE_BOOT_LEN_TRIM_DATA(dataLen)  (dataLen)

/*! \brief    extended status values. */
enum {
  BLE_EXT_STATUS_SUCCESS                  = 0x00,      /*!< No error. */
};

#endif /* BLE_BOOT_DEFS_H */
