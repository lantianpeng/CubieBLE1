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

#define SYS_LOG_DOMAIN "ota"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_WARNING
#include <logging/sys_log.h>

static struct bt_le_conn_param param;

void ota_dc_conn_param_update(struct bt_conn *conn)
{
	int err;

	/* request update to connection parameters */
	err = bt_conn_le_param_update(conn, &param);
	if (err) {
		SYS_LOG_WRN("conn update failed (err %d).", err);
	} else {
		SYS_LOG_INF("conn update initiated %d,%d,%d,%d.", param.interval_min, param.interval_max, param.latency, param.timeout);
	}
}

static u8_t ota_dc_set_conn_param_req(struct bt_conn *conn, u16_t len, u8_t *p_value)
{
	/* verify parameter length */
	if (len != OTA_DC_LEN_CONN_PARAM_REQ)
		return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;

	/* parse parameters */
	BSTREAM_TO_UINT16(param.interval_min, p_value);
	BSTREAM_TO_UINT16(param.interval_max, p_value);
	BSTREAM_TO_UINT16(param.latency, p_value);
	BSTREAM_TO_UINT16(param.timeout, p_value);

	otaCb.conn = conn;
	ota_send_event(OTA_DC_CONN_PARMA_UPDATE_BIT);

	return 0;
}

static u8_t ota_dc_get_conn_param(struct bt_conn *conn, u16_t len, u8_t *p_value)
{
	return 0;
}

u8_t ota_dc_write(struct bt_conn *conn, u16_t len, u8_t *p_value)
{
	u8_t op;
	u8_t id;
	u8_t status = 0;

	/* sanity check on message length */
	if (len < OTA_DC_HDR_LEN)
		return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;

	/* get operation and parameter ID */
	BSTREAM_TO_UINT8(op, p_value);
	BSTREAM_TO_UINT8(id, p_value);

	/* skip over header (note p_value was incremented above) */
	len -= OTA_DC_HDR_LEN;

	/* set operation */
	if (op == OTA_DC_OP_SET) {
		switch (id) {
		case OTA_DC_ID_CONN_UPDATE_REQ:
			status = ota_dc_set_conn_param_req(conn, len, p_value);
			break;
		default:
			status = BT_ATT_ERR_OUT_OF_RANGE;
			break;
		}
	} else if (op == OTA_DC_OP_GET)	{
		switch (id) {
		case OTA_DC_ID_CONN_PARAM:
			status = ota_dc_get_conn_param(conn, len, p_value);
			break;
		default:
			status = BT_ATT_ERR_OUT_OF_RANGE;
			break;
		}
	} else
		status = BT_ATT_ERR_OUT_OF_RANGE;

	return status;
}

#endif
