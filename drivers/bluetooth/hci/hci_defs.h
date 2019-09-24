/* hci_defs.h - llcc based Bluetooth driver */

/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HCI_DEFS_H
#define HCI_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Packet definitions */
#define HCI_CMD_HDR_LEN                              3       /*!< Command packet header length */
#define HCI_ACL_HDR_LEN                              4       /*!< ACL packet header length */
#define HCI_EVT_HDR_LEN                              2       /*!< Event packet header length */

/* Packet types */
#define HCI_CMD_TYPE                                 1       /*!< HCI command packet */
#define HCI_ACL_TYPE                                 2       /*!< HCI ACL data packet */
#define HCI_EVT_TYPE                                 4       /*!< HCI event packet */

/* Error codes */
#define HCI_SUCCESS                                  0x00    /*!< Success */
#define HCI_OGF_VENDOR_SPEC                          0x3F    /*!< Vendor specific */

/* Opcode manipulation macros */
#define HCI_OPCODE(ogf, ocf)                         (((ogf) << 10) + (ocf))

/* Events */
#define HCI_CMD_CMPL_EVT                             0x0E
#ifdef __cplusplus
};
#endif

#endif /* HCI_DEFS_H */
