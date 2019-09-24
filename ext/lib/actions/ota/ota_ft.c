/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#if CONFIG_OTA_WITH_APP

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <misc/printk.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "ota_profile.h"
#include "ota_defs.h"
#include "ota.h"

#include "bstream.h"
#include "ota_storage.h"
#include "boot.h"
#include "soc.h"
#include "version.h"

#define SYS_LOG_DOMAIN "ota"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_WARNING
#include <logging/sys_log.h>

#ifdef CONFIG_OTA_WITH_APP
void ota_ftc_send(struct bt_conn *conn)
{
	SYS_LOG_DBG("OTA: FTC Send");

	/* if notification enabled */
	if (ota_ccc_enabled(conn, OTA_FTC_HDL))
		ota_send_notify(conn, OTA_FTC_HDL, otaCb.ftc_msg_len, otaCb.ftc_msg_buf);

	otaCb.tx_ready_mask &= ~OTA_TX_MASK_FTC_BIT;
}

void ota_fdc_async_send(struct bt_conn *conn)
{
	u8_t *p;
	u8_t  msg_buf[20];
	u16_t msg_len;
	u16_t count = 0;

	SYS_LOG_DBG("OTA: FTC ASYNC Send");

	if (otaCb.pending_count == otaCb.send_count)
		return;

	count = otaCb.pending_count - otaCb.send_count;
	otaCb.send_count = otaCb.pending_count;

	/* build message */
	p = msg_buf;

	UINT8_TO_BSTREAM(p, OTA_FTC_OP_PACKET_RECEIVED);
	UINT16_TO_BSTREAM(p, otaCb.ft_handle);
	UINT8_TO_BSTREAM(p, 0);
	UINT16_TO_BSTREAM(p, count);

	msg_len = (u16_t) (p - msg_buf);

	/* if notification enabled */
	if (ota_ccc_enabled(conn, OTA_FTC_HDL))
		ota_send_notify(conn, OTA_FTC_HDL, msg_len, msg_buf);

	otaCb.tx_ready_mask &= ~OTA_TX_MASK_FTC_ASYNC_BIT;
}

void ota_ftc_send_rsp(struct bt_conn *conn, u8_t op, u16_t handle, u8_t status, u16_t extra_info)
{
	u8_t *p;

	 /* there should not be another response message set up */
	if (otaCb.tx_ready_mask & OTA_TX_MASK_FTC_BIT) {
		SYS_LOG_DBG("OTA: FTC message overflow op %d handle %d(%d,%d)", op, handle, otaCb.ftc_msg_buf[0], otaCb.ftc_msg_buf[1]);
		return;
	}

	SYS_LOG_DBG("OTA: FTC SendRsp op=%d handle=%d status=%d", op, handle, status);

	/* build message */
	p = otaCb.ftc_msg_buf;

	UINT8_TO_BSTREAM(p, op);
	UINT16_TO_BSTREAM(p, handle);
	UINT8_TO_BSTREAM(p, status);

	if (op == OTA_FTC_OP_PACKET_RECEIVED) {
		UINT16_TO_BSTREAM(p, extra_info);
	} else if (op == OTA_FTC_OP_GET_VERSION_RSP) {
		char *version = sys_version_get();

		memcpy(p, version, strlen(version));
		p = p + strlen(version);
	}

	otaCb.ftc_msg_len = (u16_t) (p - otaCb.ftc_msg_buf);

	/* Indicate TX Ready */
	otaCb.tx_ready_mask |= OTA_TX_MASK_FTC_BIT;
	otaCb.conn = conn;
	ota_send_event(OTA_TX_MASK_FTC_BIT);
}


static void ota_ftc_proc_get_req(struct bt_conn *conn, u16_t handle, u16_t len, u8_t *p_value)
{
	u8_t status;

	SYS_LOG_DBG("OTA: FTC GetReq handle=%d len=%d", handle, len);

	/* verify operation not already in progress */
	if (otaCb.ft_in_progress != OTA_FTC_OP_NONE)
		status = OTA_FTC_ST_IN_PROGRESS;
	else {
		/* parse operation data */
		BSTREAM_TO_UINT32(otaCb.ft_addr, p_value);
		BSTREAM_TO_UINT32(otaCb.ft_len, p_value);
		if (handle == 0) {
			otaCb.ft_addr = sys_read32(PARTITION_TABLE_ADDR);
			otaCb.ft_len = sizeof(struct partition_table);
#ifdef CONFIG_SPI0_XIP
#define SPI0_XIP_CRC_ALIGN(x) ((x) * 34 / 32)
			otaCb.ft_len = SPI0_XIP_CRC_ALIGN(otaCb.ft_len);
#endif
		}

	SYS_LOG_DBG("OTA: FTC GetReq ft_addr=%d ft_len=%d", otaCb.ft_addr, otaCb.ft_len);


		/* set up file get operation */
		otaCb.ft_handle = handle;
		otaCb.ft_in_progress = OTA_FTC_OP_GET_REQ;

		status = OTA_FTC_ST_SUCCESS;
	}

	/* send response */
	ota_ftc_send_rsp(conn, OTA_FTC_OP_GET_RSP, handle, status, 0);

	if (status == OTA_FTC_ST_SUCCESS) {
		otaCb.conn = conn;
		ota_send_event(OTA_TX_MASK_FTD_BIT);
	}
}

static void ota_ftc_proc_put_req(struct bt_conn *conn, u16_t handle, u16_t len, u8_t *p_value)
{
	u8_t status;

	/* verify operation not already in progress */
	if (otaCb.ft_in_progress != OTA_FTC_OP_NONE)
		status = OTA_FTC_ST_IN_PROGRESS;
	else {
		/* parse operation data */
		otaCb.ft_handle = handle;
		BSTREAM_TO_UINT32(otaCb.ft_addr, p_value);
		BSTREAM_TO_UINT32(otaCb.ft_len, p_value);

		SYS_LOG_DBG("OTA: FTC PutReq handle=%d offset=%d, len=%d", handle,
		otaCb.ft_addr, otaCb.ft_len);

		/* set up file put operation */
		otaCb.ft_handle = handle;
		otaCb.ft_in_progress = OTA_FTC_OP_PUT_REQ;

		/* let apk/app to do it */
		/* ota_storage_erase(otaCb.storage, otaCb.ft_addr, ((otaCb.ft_len + 0x1000 - 1) / 0x1000) * 0x1000); */
		otaCb.pending_count = 0;
		otaCb.send_count = 0;

		status = OTA_FTC_ST_SUCCESS;
	}

	SYS_LOG_DBG("OTA: FTC PutReq handle=%d status=%d", handle, status);

	/* send response */
	ota_ftc_send_rsp(conn, OTA_FTC_OP_PUT_RSP, handle, status, 0);
}
#define OTA_MIN(a, b)        ((a) < (b) ? (a) : (b))
static void ota_ftc_proc_verify_req(struct bt_conn *conn, u16_t handle, u16_t len, u8_t *p_value)
{
	u8_t status;

	SYS_LOG_DBG("OTA: FTC VerifyReq: handle=%d", handle);
	/* verify operation not already in progress */
	if (otaCb.ft_in_progress != OTA_FTC_OP_NONE)
		status = OTA_FTC_ST_IN_PROGRESS;
	else {
		/* parse operation data */
		otaCb.ft_handle = handle;
		BSTREAM_TO_UINT32(otaCb.ft_addr, p_value);
		BSTREAM_TO_UINT32(otaCb.ft_len, p_value);
		BSTREAM_TO_UINT32(otaCb.ft_crc, p_value);

		u32_t off = 0;
		u32_t crc = 0;
		u32_t read_len;
		u8_t pageBuf[256];

		/* Verify image data. */
		while (off < otaCb.ft_len) {
			read_len = OTA_MIN(256, otaCb.ft_len - off);
			ota_storage_read(otaCb.storage, otaCb.ft_addr + off, pageBuf, read_len);
			crc = crc32(pageBuf, read_len, crc);
			off += read_len;
		}

		status = (crc == otaCb.ft_crc) ? OTA_FTC_ST_SUCCESS : OTA_FTC_ST_VERIFICATION;
	}

	/* send response */
	ota_ftc_send_rsp(conn, OTA_FTC_OP_VERIFY_RSP, handle, status, 0);
}

static void ota_ftc_proc_erase_req(struct bt_conn *conn, u16_t handle, u16_t len, u8_t *p_value)
{
	u8_t status = OTA_FTC_ST_SUCCESS;

	SYS_LOG_DBG("OTA: FTC EraseReq: handle=%d", handle);

	/* verify operation not already in progress */
	if (otaCb.ft_in_progress != OTA_FTC_OP_NONE)
		status = OTA_FTC_ST_IN_PROGRESS;
	else {
		/* parse operation data */
		otaCb.ft_handle = handle;
		BSTREAM_TO_UINT32(otaCb.ft_addr, p_value);
		BSTREAM_TO_UINT32(otaCb.ft_len, p_value);
		/* do file erase */
		ota_storage_erase(otaCb.storage, otaCb.ft_addr, otaCb.ft_len);
	}

	/* send response */
	ota_ftc_send_rsp(conn, OTA_FTC_OP_ERASE_RSP, handle, status, 0);
}

static void ota_ftc_proc_abort(struct bt_conn *conn, u16_t handle)
{
	SYS_LOG_DBG("OTA: FTC AbortReq: handle=%d", handle);

	if (otaCb.ft_in_progress != OTA_FTC_OP_NONE) {
		/* abort operation */
		otaCb.ft_in_progress = OTA_FTC_OP_NONE;

		otaCb.ft_len = 0;
		otaCb.ft_addr = 0;
	}
}

static void ota_ftc_proc_get_version_req(struct bt_conn *conn, u16_t handle)
{
	u8_t status = OTA_FTC_ST_SUCCESS;

	SYS_LOG_DBG("OTA: FTC GetVersionReq: handle=%d", handle);

	/* send response */
	ota_ftc_send_rsp(conn, OTA_FTC_OP_GET_VERSION_RSP, handle, status, 0);
}

u8_t ota_need_reset;
static void ota_ftc_proc_reset(struct bt_conn *conn, u16_t handle)
{
	SYS_LOG_DBG("OTA: FTC ResetReq: handle=%d", handle);
  #ifdef CONFIG_OTA_WITH_APP
	ota_need_reset = 1;
  #endif
	/* send disconnect msg to deal with */
	ota_send_event(OTA_DC_CONN_DISCONNECT_BIT);
}

u8_t ota_ftd_write(struct bt_conn *conn, u16_t len, u8_t *p_value)
{
	SYS_LOG_DBG("OTA: FTD write: len=%d", len);

	/* verify put operation in progress */
	if (otaCb.ft_in_progress != OTA_FTC_OP_PUT_REQ)
		return BT_ATT_ERR_UNLIKELY;

	/* verify data length */
	if (len <= OTA_FTD_HDR_LEN)
		return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;

	/* verify more data is expected */
	if (otaCb.ft_len >= len) {
		ota_storage_write(otaCb.storage, otaCb.ft_addr, p_value, len);

		/* update remaining length of put request */
		otaCb.ft_addr += len;
		otaCb.ft_len -= len;

		/* send packet received */
		otaCb.pending_count += len;
		otaCb.conn = conn;

		/* if end of put req reached */
		if (otaCb.ft_len == 0) {
			/* put req done */
			otaCb.ft_in_progress = OTA_FTC_OP_NONE;

			ota_send_event(OTA_TX_MASK_FTC_ASYNC_BIT);

			/* send eof */
			ota_ftc_send_rsp(conn, OTA_FTC_OP_EOF, otaCb.ft_handle, 0, 0);
		} else {
			if (!(otaCb.tx_ready_mask & OTA_TX_MASK_FTC_ASYNC_BIT)) {
				otaCb.tx_ready_mask |= OTA_TX_MASK_FTC_ASYNC_BIT;
				ota_send_event(OTA_TX_MASK_FTC_ASYNC_BIT);
			}
		}
	}

	return 0;
}

u8_t ota_ftc_write(struct bt_conn *conn, u16_t len, u8_t *p_value)
{
	u8_t   op;
	u16_t  handle;

	SYS_LOG_DBG("OTA: FTC Write: len=%d", len);

	/* sanity check on message length */
	if (len < OTA_FTC_HDR_LEN + OTA_FTC_HANDLE_LEN)
		return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;

	/* get operation and file handle */
	BSTREAM_TO_UINT8(op, p_value);
	BSTREAM_TO_UINT16(handle, p_value);

	SYS_LOG_DBG("OTA: FTC Write: op=%d handle=%d", op, handle);

	len -= OTA_FTC_HANDLE_LEN + OTA_FTC_HDR_LEN;

	switch (op) {
	case OTA_FTC_OP_GET_REQ:
		ota_ftc_proc_get_req(conn, handle, len, p_value);
		break;

	case OTA_FTC_OP_PUT_REQ:
		ota_ftc_proc_put_req(conn, handle, len, p_value);
		break;

	case OTA_FTC_OP_VERIFY_REQ:
		ota_ftc_proc_verify_req(conn, handle, len, p_value);
		break;

	case OTA_FTC_OP_ERASE_REQ:
		ota_ftc_proc_erase_req(conn, handle, len, p_value);
		break;

	case OTA_FTC_OP_ABORT:
		ota_ftc_proc_abort(conn, handle);
		break;

	case OTA_FTC_OP_SYSTEM_RESET:
		ota_ftc_proc_reset(conn, handle);
		break;

	case OTA_FTC_OP_GET_VERSION_REQ:
		ota_ftc_proc_get_version_req(conn, handle);
		break;

	default:
		break;
	}

	return 0;
}

void ota_ftd_send(struct bt_conn *conn)
{
	/* if notification enabled */
	if (ota_ccc_enabled(conn, OTA_FTD_HDL)) {
	u8_t   buf[20];
	u8_t   *p_buf = buf;
	u32_t  readLen = 20; /* bt_gatt_get_mtu(conn) - 3; */

	/* read data from file */
	readLen = (readLen < otaCb.ft_len) ? readLen : otaCb.ft_len;

	if (readLen)
		ota_storage_read(otaCb.storage, otaCb.ft_addr, p_buf, readLen);

	/* if read data length not zero */
	if (readLen > 0) {
	/* update stored offset and length (non-streaming file) */
	otaCb.ft_len -= readLen;
	otaCb.ft_addr += readLen;

	/* send notification */
	ota_send_notify(conn, OTA_FTD_HDL, (u16_t)readLen, buf);
	}

	/* check if end of transfer reached */
	if (otaCb.ft_len == 0 || readLen == 0) {
		otaCb.ft_in_progress = OTA_FTC_OP_NONE;
			otaCb.tx_ready_mask &= ~(OTA_TX_MASK_FTD_BIT);
			ota_ftc_send_rsp(conn, OTA_FTC_OP_EOF, otaCb.ft_handle, 0, 0);
	} else {
			otaCb.conn = conn;
			ota_send_event(OTA_TX_MASK_FTD_BIT);
		}
	}
}
#endif
#endif
