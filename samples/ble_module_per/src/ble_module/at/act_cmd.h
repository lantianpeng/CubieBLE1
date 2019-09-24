/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/util.h>

#define ACT_CMD_MTU 256
#define ACT_CMD_DATA_MAX_SIZE (ACT_CMD_MTU - sizeof(struct act_cmd_hdr))

#define ACT_CMD_INDEX_NONE		0xff

/* Packet definitions */
#define ACT_ACL_HDR_LEN                              4       /*!< Actions ACL packet header length */
#define ACT_CMD_HDR_LEN                              3       /*!< Actions Command packet header length */
#define ACT_EVT_HDR_LEN                              2       /*!< Actions Event packet header length */

/* Packet types */
#define ACT_CMD_TYPE                                 0xA1       /*!< Actions command packet */
#define ACT_ACL_TYPE                                 0xA2       /*!< Actions extend command packet */
#define ACT_EVT_TYPE                                 0xA4       /*!< Actions event packet */

/* Error codes */
#define ACT_SUCCESS                                  0x00    /*!< Success */
#define ACT_ERR_UNKNOWN_CMD                          0x01    /*!< Unknown Actions command */
#define ACT_ERR_DATA_LEN                             0x02    /*!< the length of data not match */
#define ACT_ERR_INVALID_DATA                         0x03    /*!< the length of data not match */
#define ACT_ERR_BLE_STATE                            0x04    /*!< cmd don't work for this BLE STATE */
#define ACT_ERR_ACK_TIMEOUT                          0x05    /*!< cmd ack timeout */
#define ACT_ERR_OP_FAIL                              0x06    /*!< module can't deal with this cmd */
#define ACT_ERR_NO_MEMORY                            0x07    /*!< module haven't more memory do deal with this cmd */

/* Command groups */
#define ACT_OGF_VENDOR_SPEC                          0x3F    /*!< Vendor specific */

/* Opcode manipulation macros */
#define ACT_OPCODE(ogf, ocf)                         (((ogf) << 10) + (ocf))
#define ACT_OGF(opcode)                              ((opcode) >> 10)
#define ACT_OCF(opcode)                              ((opcode) & 0x03FF)

#define ACT_CMD_DONE_EVT                             0x0D
#define ACT_CMD_ERROR_EVT                            0x0E
#define ACT_CMD_ASYNC_EVT                            0xDD  /* for some async cmd such as RSSI */

/* Cordio host vendor specific OCF range is 0x000-0x00F. */
#define CMD_HOST_OCF_BASE                      0x300
#define CMD_HOST_OCF_END                       0x32F

#define CMD_READ_WORK_MODE            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x04)  /*!<  */
#define CMD_WRITE_WORK_MODE           ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x05)  /*!<  */
#define CMD_SOFT_RESET                ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x06)  /*!<  */
#define CMD_READ_DEVICE_NAME          ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x07)  /*!<  */
#define CMD_WRITE_DEVICE_NAME         ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x08)  /*!<   */
#define CMD_WRITE_DEF_TX_POWER        ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x11)  /*!<  */
#define CMD_WRITE_TX_POWER            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x12)  /*!<  */
#define CMD_READ_CONN_PARAM           ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x13)  /*!<  */
#define CMD_WRITE_CONN_PARAM          ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x14)  /*!<  */
#define CMD_READ_MAX_CONN             ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x15)  /*!<  */
#define CMD_WRITE_MAX_CONN            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x16)  /*!<  */
#define CMD_READ_BD_ADDR              ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x17)  /*!<  */
#define CMD_WRITE_BD_ADDR             ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x18)  /*!<  */
#define CMD_READ_ADV_DATA             ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x19)  /*!<  */
#define CMD_WRITE_ADV_DATA            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x1A)  /*!<  */
#define CMD_READ_ADV_INTERVAL         ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x1B)  /*!<  */
#define CMD_WRITE_ADV_INTERVAL        ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x1C)  /*!<  */
#define CMD_READ_RSSI                 ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x1D)  /*!<  */
#define CMD_READ_BLE_STATE            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x1E)  /*!<  */
#define CMD_BLE_DISCONNECT            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x1F)  /*!<  */
#define CMD_READ_SCAN_DATA            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x20)  /*!<  */
#define CMD_WRITE_SCAN_DATA           ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x21)  /*!<  */

#define CMD_READ_UARTBAUD             ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x30)  /*!<  */
#define CMD_WRITE_UARTBAUD            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x31)  /*!<  */
#define CMD_READ_UARTMODE             ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x32)  /*!<  */
#define CMD_WRITE_UARTMODE            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x33)  /*!<  */
#define CMD_READ_GPIO_WAKEUP_CFG      ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x34)  /*!<  */
#define CMD_WRITE_GPIO_WAKEUP_CFG     ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x35)  /*!<  */
#define CMD_FACTORY_RESET             ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x36)  /*!<  */
#define CMD_WRITE_HCL_32KMODE         ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x37)  /*!<  */
#define CMD_WRITE_APP_MODE            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x38)  /*!<  */
#define CMD_WRITE_HOST_CFG_COMPLETED  ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x39)  /*!<  */
#define CMD_WRITE_EXT_IRQ_CLOCK       ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x3A)  /*!<  */
#define CMD_WRITE_SLEEP_CLK_ACCURACY  ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x3B)  /*!<  */
#define CMD_WRITE_BOARD_TRIM_DATA     ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x3C)  /*!<  */
#define CMD_WRITE_TX_TRIM_DATA        ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x3D)  /*!<  */
#define CMD_WRITE_TX_POWER_LEVEL_3V   ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x3E)  /*!<  */

#define CMD_TXDATA_CMDMODE            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x40)  /*!<  */
#define CMD_SET_BT4PATCH_ENTRY        ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x41)  /*!<  */
#define CMD_READ_GPIO                 ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x42)  /*!<  */
#define CMD_WRITE_GPIO                ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x43)  /*!<  */
#define CMD_WRITE_UUID                ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x44)  /*!<  */
#define CMD_DOWNLOAD_PATCH            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x45)  /*!<  */
#define CMD_EXEC_PATCH                ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x46)  /*!<  */

#define CMD_SWITCH_BATCH              ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x50)  /*!<  */
#define CMD_WRITE_COMP_ID             ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x51)  /*!<  */
#define CMD_WRITE_IMPL_REV            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x52)  /*!<  */
#define CMD_WRITE_BT_VER              ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x53)  /*!<  */
#define CMD_WRITE_PWR_MODE            ACT_OPCODE(ACT_OGF_VENDOR_SPEC, CMD_HOST_OCF_BASE + 0x54)  /*!<  */

#define CMD_INQUIRY_STATUS            0xcc00
#define CMD_WRITE_RAM                 0xcd13
#define CMD_SWITCH_FIRMWARE           0xcd20
#define CMD_READ_RAM                  0xcd93

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define SYS_LOG_DOMAIN "act_cmd"
#include <logging/sys_log.h>

struct act_cmd_hdr {	
	u8_t  cmd_type;
	u16_t opcode;
	u16_t len;
	u8_t  data[0];
} __packed;

#define ACT_CMD_STATUS			0x00
struct act_cmd_status {
	u8_t code;
} __packed;



/* events */
#define CORE_EV_IUT_READY		0x80


void act_cmd_init(void);
void act_cmd_rsp(u8_t service, u16_t opcode, u8_t status);
void act_cmd_send(u8_t cmd_type, u16_t opcode, u8_t *data, size_t len);


