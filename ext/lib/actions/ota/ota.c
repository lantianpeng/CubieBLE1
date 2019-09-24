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

#include "soc_pm.h"
#include "ota_profile.h"
#include "ota.h"
#include "ota_storage.h"

#include "ota_defs.h"
#include "msg_manager.h"

#define SYS_LOG_DOMAIN "ota"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_WARNING
#include <logging/sys_log.h>

struct otaCb_t otaCb;
#define OTA_WAKEUP_TIMEOUT K_SECONDS(60 * 2) /* 60s * 2 */

void ota_send_event(u32_t event)
{
	struct app_msg  msg = {0};

	msg.type = MSG_OTA_EVENT;
	msg.value = event;

	send_msg(&msg, K_MSEC(100));
}

void ota_event_handle(u32_t event)
{
	switch (event) {
	case OTA_TX_MASK_FTD_BIT:
		ota_ftd_send(otaCb.conn);
		break;
	case OTA_TX_MASK_FTC_BIT:
		ota_ftc_send(otaCb.conn);
		break;
	case OTA_TX_MASK_FTC_ASYNC_BIT:
		ota_fdc_async_send(otaCb.conn);
		break;
	case OTA_DC_CONN_PARMA_UPDATE_BIT:
		ota_dc_conn_param_update(otaCb.conn);
		break;
	case OTA_DC_CONN_DISCONNECT_BIT:
		bt_conn_disconnect(otaCb.conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		break;
	}
}

u8_t ota_write_cback(struct bt_conn *conn, u16_t handle, u8_t operation,
	u16_t offset, u16_t len, u8_t *p_value, const struct bt_gatt_attr *attr)
{
	u8_t status;

	/* set wakeup timer when wakeup by ble event from peer device */
	if (otaCb.wakeup_flag == false) {
		app_get_wake_lock();
		k_timer_start(&otaCb.wakeup_timeout_timer, OTA_WAKEUP_TIMEOUT, 0);
		otaCb.wakeup_flag = true;
		SYS_LOG_INF("ota start wakeup_timer");
	}

	switch (handle) {
	/* Device configuration */
	case OTA_DC_HDL:
		status = ota_dc_write(conn, len, p_value);
		break;

	/* File transfer control */
	case OTA_FTC_HDL:
		status = ota_ftc_write(conn, len, p_value);
		break;

	/* File transfer data */
	case OTA_FTD_HDL:
		status = ota_ftd_write(conn, len, p_value);
		break;

	default:
		SYS_LOG_WRN("OTA: WriteCback unexpected handle=%d", handle);
		status = BT_ATT_ERR_INVALID_HANDLE;
		break;
	}

	return status;
}

void ota_wakeup_timeout_handler(struct k_timer *timer)
{
	if (otaCb.wakeup_flag) {
		app_release_wake_lock();
		otaCb.wakeup_flag = false;
		SYS_LOG_INF("ota stop wakeup_timer");
	}
}

__init_once_text void ota_profile_init(void)
{
	SYS_LOG_INF("OTA: ota_profile_init");

	/* Initialize the control block */
	memset(&otaCb, 0, sizeof(otaCb));

	/* Register the OTA Service */
	ota_register(ota_write_cback);
	ota_init();

	otaCb.storage = ota_storage_init();

	/* init ota wakeup timer */
	k_timer_init(&otaCb.wakeup_timeout_timer, ota_wakeup_timeout_handler, NULL);
}

#endif
