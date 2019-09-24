/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef OTA_H
#define OTA_H

#include <zephyr.h>

/* TX Ready Mask Bits */
#define OTA_TX_MASK_READY_BIT						(1<<0)
#define OTA_TX_MASK_DC_BIT							(1<<1)
#define OTA_TX_MASK_FTC_BIT							(1<<2)
#define OTA_TX_MASK_FTD_BIT							(1<<3)
#define OTA_TX_MASK_AU_BIT							(1<<4)
#define OTA_TX_MASK_FTC_ASYNC_BIT				(1<<5)
#define OTA_DC_CONN_PARMA_UPDATE_BIT		(1<<6)
#define OTA_DC_CONN_DISCONNECT_BIT			(1<<7)

struct otaCb_t {
	u8_t           tx_ready_mask;      /* Bits indicate DC, FTC, FTD, and/or AU wish to transmit */

	/* for file transfer */
	u32_t          ft_addr;        /* data addr */
	u32_t          ft_len;           /* remaining data length for current operation */
	u32_t          ft_crc;       		/* crc */
	u16_t          ft_handle;         /* file handle */
	u16_t          ftc_msg_len;        /* message length */
	u8_t           ftc_msg_buf[20];    /* message buffer */

	u8_t           ft_in_progress;     /* operation in progress */
	u8_t 					 wakeup_flag;
	u32_t pending_count;
	u32_t send_count;
	struct bt_conn *conn;
	struct device *storage;
	struct k_timer wakeup_timeout_timer;
};

extern struct otaCb_t otaCb;

/**
 *  @brief  send ftc response to peer device Synchronously
 *
 *
 *  @param  conn			: current ota connection
 *
 */

void ota_ftc_send(struct bt_conn *conn);

/**
 *  @brief  send ftc response to peer device asynchronously
 *
 *
 *  @param  conn			: current ota connection
 *
 */

void ota_fdc_async_send(struct bt_conn *conn);

/**
 *  @brief  send data through ftd to peer device
 *
 *
 *  @param  conn			: current ota connection
 *
 */

void ota_ftd_send(struct bt_conn *conn);

/**
 *  @brief  deal with conn param update request from dc attribute
 *
 *
 *  @param  conn			: current ota connection
 *
 */

void ota_dc_conn_param_update(struct bt_conn *conn);

/**
 *  @brief  ota dc attribute write data request
 *
 *
 *  @param  conn			: current ota connection
 *  @param  len				: size of data to be write
 *  @param  p_value		: data buffer to write
 *
 *  @return  0 on success, negative errno code on fail.
 */

u8_t ota_dc_write(struct bt_conn *conn, u16_t len, u8_t *p_value);

/**
 *  @brief  ota ftc attribute write data request
 *
 *
 *  @param  conn			: current ota connection
 *  @param  len				: size of data to be write
 *  @param  p_value		: data buffer to write
 *
 *  @return  0 on success, negative errno code on fail.
 */

u8_t ota_ftc_write(struct bt_conn *conn, u16_t len, u8_t *p_value);

/**
 *  @brief  ota ftd attribute write data request
 *
 *
 *  @param  conn			: current ota connection
 *  @param  len				: size of data to be write
 *  @param  p_value		: data buffer to write
 *
 *  @return  0 on success, negative errno code on fail.
 */

u8_t ota_ftd_write(struct bt_conn *conn, u16_t len, u8_t *p_value);

/**
 *  @brief  ota profile initilization
 *
 *
 *
 */

void ota_profile_init(void);

/**
 *  @brief  ota event handler
 *
 *
 *  @param  event 
 *
 */

void ota_event_handle(u32_t event);

/**
 *  @brief  send ota event to ota event handler
 *
 *
 *  @param  event 
 *
 */

void ota_send_event(u32_t event);

extern u32_t crc32(const u8_t *data, int len, u32_t init_vect);
#endif /* OTA_H */
