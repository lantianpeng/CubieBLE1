/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: lipeng<lipeng@actions-semi.com>
 *
 * Change log:
 *	2017/5/10: Created by lipeng.
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <misc/printk.h>
#include <zephyr.h>

#include "soc_pm.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/gatt.h>

#include "wechat_service.h"
#include "wechat_protocol.h"
#include "mem_manager.h"

#define SYS_LOG_DOMAIN "airsync"
#define SYS_LOG_LEVEL 4//CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

#define RX_PKT_CACHE_LEN	(384)
#define RESULT_BUFFER_LEN	(128)

#define HEXDUMP_ENABLEX

enum airsync_app_status {
	APP_STATUS_INIT = 0,
	APP_STATUS_RUNNING,
	APP_STATUS_STOPPED,
};

enum airsync_proto_status {
	PROTO_STATUS_CAN_NOT_IND = 0,
	PROTO_STATUS_IDLE,
	PROTO_STATUS_AUTH_SENDING,
	PROTO_STATUS_AUTH_RSP_RECEIVED,
	PROTO_STATUS_INIT_SENDING,
	PROTO_STATUS_INIT_RSP_RECEIVED,
	PROTO_STATUS_SEND_OR_RECV,
};

struct airsync_priv {
	enum airsync_proto_status	proto_status;
	struct k_mutex			proto_status_mutex;

	/* a packet buffer to cache a complete pacet */
	uint8_t				pkt_cache[RX_PKT_CACHE_LEN];
	uint16_t			cached_len;

	/* result buffer to save SSID & key */
	uint8_t				result_buf[RESULT_BUFFER_LEN];
	bool				got_result;
};

struct airsync_priv airsync_priv_data;
/*===========================================================================
 *			Global static variables
 *=========================================================================*/

static struct airsync_priv			*as = &airsync_priv_data;

/*===========================================================================
 *				wechat airsync
 *=========================================================================*/
static int airsync_proto_recv_cb(void *buf, int len)
{
	uint8_t *raw_data;
	uint32_t raw_len;

	int ret;

	SYS_LOG_DBG("buf %p, len %d\n", buf, len);

	if (as->cached_len > 0) {
		/* merge the new data to cached data, and parsed them. */

		if (as->cached_len + len > RX_PKT_CACHE_LEN) {
			SYS_LOG_ERR("cached buffer full!! discard!!!\n");
			as->cached_len = 0;
			return len;
		}

		memcpy(&as->pkt_cache[as->cached_len], buf, len);
		as->cached_len += len;

		ret = data_consume_func_rec(as->pkt_cache, as->cached_len, &raw_data, &raw_len);
	} else {
		/* try to parsed the packet directly. */
		ret = data_consume_func_rec(buf, len, &raw_data, &raw_len);
	}


	if (ret == 0) {
		SYS_LOG_DBG("need more data(cached: %d)\n", as->cached_len);

		/*
		 * need more data, cached it
		 * NOTE: if as->cached_len > 0, the packet is alredy cached.
		 */
		if (as->cached_len == 0) {
			memcpy(&as->pkt_cache[as->cached_len], buf, len);
			as->cached_len += len;
		}
		return len;
	} else if (ret < 0) {
		SYS_LOG_ERR("data_consume_func_rec (err %d, cached %d)\n", ret, as->cached_len);

		/* dicard all cached data */
		as->cached_len = 0;
		return ret;
	} else {
		SYS_LOG_DBG("success comsumed one packet(cached: %d, consumed %d)\n", as->cached_len, ret);

		/* remove the cached data */
		if (as->cached_len > ret) {
			memcpy(as->pkt_cache, &as->pkt_cache[ret], as->cached_len - ret);
			as->cached_len -= ret;
		} else {
			as->cached_len = 0;
		}
	}

	k_mutex_lock(&as->proto_status_mutex, K_FOREVER);

	switch (as->proto_status) {
	case PROTO_STATUS_AUTH_SENDING:
		as->proto_status = PROTO_STATUS_AUTH_RSP_RECEIVED;
		break;

	case PROTO_STATUS_INIT_SENDING:
		as->proto_status = PROTO_STATUS_INIT_RSP_RECEIVED;
		break;

	case PROTO_STATUS_SEND_OR_RECV:
	{
		SYS_LOG_DBG("%d received\n", raw_len);
		break;
	}

	default:
		SYS_LOG_WRN("weared data?\n");
	}
	k_mutex_unlock(&as->proto_status_mutex);

	return len;
}

static void airsync_proto_ind_status_cb(bool onoff)
{
	SYS_LOG_DBG("status %d\n", onoff);

	if (!as)
		return;

	k_mutex_lock(&as->proto_status_mutex, K_FOREVER);

	if (onoff)
		as->proto_status = PROTO_STATUS_IDLE;
	else
		as->proto_status = PROTO_STATUS_CAN_NOT_IND;

	mem_init();
	as->cached_len = 0;
	k_mutex_unlock(&as->proto_status_mutex);

}

static void airsync_proto_handle(struct airsync_priv *as)
{
	cmd_parameter_t param;
	uint8_t *buf;
	uint32_t len = 0;

	k_mutex_lock(&as->proto_status_mutex, K_FOREVER);

	/* Wechat does not enable indciation, do nothing... */
	if (as->proto_status == PROTO_STATUS_CAN_NOT_IND)
		goto sleep_out;

	switch (as->proto_status) {
	case PROTO_STATUS_IDLE:
		SYS_LOG_DBG("CMD_AUTH\n");

		as->proto_status = PROTO_STATUS_AUTH_SENDING;
		param.cmd = CMD_AUTH;
		data_produce_func_send(&param, &buf, &len);
		wechat_send_data(buf, len);
		mem_free(buf);
		break;

	case PROTO_STATUS_AUTH_RSP_RECEIVED:
		SYS_LOG_DBG("CMD_INIT\n");

		as->proto_status = PROTO_STATUS_INIT_SENDING;
		param.cmd = CMD_INIT;
	  data_produce_func_send(&param, &buf, &len);
		wechat_send_data(buf, len);
		mem_free(buf);
		break;

	case PROTO_STATUS_INIT_RSP_RECEIVED:
		SYS_LOG_DBG("INIT_RSP_RECEIVED\n");
		as->proto_status = PROTO_STATUS_SEND_OR_RECV;
		//test CMD_SENDDAT
		{
			param.cmd = CMD_SENDDAT;

			char data[] = {0x11, 0x22, 0x33, 0x44};
			param.send_msg.str = data;
			param.send_msg.len = 4;

			data_produce_func_send(&param, &buf, &len);
			wechat_send_data(buf, len);
			mem_free(buf);
		}
		break;

	default:
		goto sleep_out;
	}

	k_mutex_unlock(&as->proto_status_mutex);
	return;

sleep_out:
	k_mutex_unlock(&as->proto_status_mutex);
	k_sleep(100);
}

__init_once_text static void bt_ready(int err)
{
	uint8_t mac_addr[6];
	struct bt_le_oob oob;
	
	/* don't enter into deepsleep when bt stack init */
	app_release_wake_lock();

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	
	printk("Bluetooth initialized\n");

	err = bt_le_oob_get_local(&oob);
	if (err) {
		SYS_LOG_ERR("get local oob failed (err %d)\n", err);
		return ;
	}
	/* mac address which is big endian */
	mac_addr[0] = oob.addr.a.val[5];
	mac_addr[1] = oob.addr.a.val[4];
	mac_addr[2] = oob.addr.a.val[3];
	mac_addr[3] = oob.addr.a.val[2];
	mac_addr[4] = oob.addr.a.val[1];
	mac_addr[5] = oob.addr.a.val[0];
	wx_set_mac_address(mac_addr);

	err = wechat_init(airsync_proto_recv_cb, airsync_proto_ind_status_cb);
	if (err) {
		SYS_LOG_ERR("wechat_init failed (err %d)\n", err);
		return ;
	}

	err = wechat_advertise(true);
	if (err) {
		SYS_LOG_ERR("wechat_advertise failed (err %d)\n", err);
		return ;
	}

	as->proto_status = PROTO_STATUS_CAN_NOT_IND;
	as->cached_len = 0;

	k_mutex_init(&as->proto_status_mutex);
}


/* main loop for airsync recognizer app */
void app_main(void)
{
	int err;

	SYS_LOG_INF("enter airsync\n");
	app_get_wake_lock();
	err = bt_enable(bt_ready);
	if (err) {
		app_release_wake_lock();
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	SYS_LOG_DBG("airsync_proto_handle...\n");
	while (1) {
		airsync_proto_handle(as);
	}
}
