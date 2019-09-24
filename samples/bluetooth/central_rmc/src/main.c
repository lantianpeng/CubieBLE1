/** @file
 *  @brief BLE Master
 */

/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <misc/byteorder.h>
#include "msg_manager.h"

#define RMC_NAME "BLE_RMC"
#define RMC_SECURITY_LEVEL BT_SECURITY_MEDIUM
#define RMC_SECURITY_TRY_MAX_TIMES 3
#define DUP_ENABLE  BT_HCI_LE_SCAN_FILTER_DUP_ENABLE	
#define DUP_DISABLE BT_HCI_LE_SCAN_FILTER_DUP_DISABLE	

#define SVC_MAX_CNT   8
#define CHRC_MAX_CNT  12
#define SUBSCRIBE_RMC_ONLY 1

struct ble_chrc {
	union {
		struct bt_uuid_16 uuid16;
	    struct bt_uuid_128 uuid128;
	} uuid;
	u16_t	handle;
	u8_t	properties;
	struct  bt_gatt_subscribe_params subscribe_params;
};

struct ble_service {
    u8_t type;
	union {
		struct bt_uuid_16 uuid16;
	    struct bt_uuid_128 uuid128;
	} uuid;
    u16_t start_handle;
    u16_t end_handle;
	u16_t chrc_cnt;
	struct ble_chrc chrc[CHRC_MAX_CNT];
};

int svc_cnt;
struct ble_service ble_services[SVC_MAX_CNT];

struct bt_le_scan_param scan_param = {
	.type       = BT_HCI_LE_SCAN_ACTIVE,   // direct ad can not found use active scan mode
	.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
	.interval   = BT_GAP_SCAN_FAST_INTERVAL,
	.window     = BT_GAP_SCAN_FAST_WINDOW
};

/* Vendor Primary Service UUID */
static struct bt_uuid_128 wp_uuid = BT_UUID_INIT_128(
	0x78, 0x56, 0x34, 0x12, 0x99, 0x88, 0x77, 0x66,
	0x55, 0x44, 0x33, 0x22, 0xA0, 0x20, 0x11, 0x00);

struct bt_conn *default_conn;
static struct bt_gatt_discover_params discover_params;
static u8_t security_err_cnt = 0;
extern bool bt_addr_le_is_bonded(const bt_addr_le_t *addr);
extern struct bt_keys *bt_keys_find_addr(const bt_addr_le_t *addr);
extern void bt_keys_clear(struct bt_keys *keys);
static u8_t notify_func(struct bt_conn *conn,
		struct bt_gatt_subscribe_params *params,
		const void *data, u16_t length);

static void send_async_msg(u8_t msg_type)
{
	struct app_msg msg = {0};
	msg.type = msg_type;
	send_msg(&msg, K_NO_WAIT);
}

void service_init(void)
{
	int i;
	for (i = 0; i < SVC_MAX_CNT; i++) {
		ble_services[i].chrc_cnt = 0;
	}
	svc_cnt = 0;
}

static u8_t notify_func(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, u16_t length)
{
	if (length) {
		int i;
		//printk("handle %s notification: ", params->value_handle);
		
		/* comment out if (length == 3), if you want to print all notification data */
		if (length == 3) {
			for (i = 0; i < length; i++) {
				printk("%02x ", ((u8_t *)data)[i]);
			}
			printk("\n ");
		}
	}

	if (!data) {
		printk("Unsubscribed\n");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

void pair(void) {
	int err;
	if (!default_conn) {
		printk("Not connected\n");
		return;
	}

	printk("Start security level 2\n");
	err = bt_conn_security(default_conn, RMC_SECURITY_LEVEL);
	if (err) {
		printk("Failed to set security\n");
	}
}

static void nofity_enable(void)
{
	int i, j, err, ccc_cnt = 0;

	for(i = 0; i < svc_cnt; i++) {
#if SUBSCRIBE_RMC_ONLY
		if (bt_uuid_cmp(BT_UUID_HIDS, &ble_services[i].uuid) && 
				bt_uuid_cmp(&wp_uuid, &ble_services[i].uuid)) {
			continue;
		}
#endif 		
		for (j = 0; j < ble_services[i].chrc_cnt; j++) {
			if ((ble_services[i].chrc[j].properties & BT_GATT_CHRC_NOTIFY) || 
				(ble_services[i].chrc[j].properties & BT_GATT_CHRC_INDICATE)) {				
				ble_services[i].chrc[j].subscribe_params.ccc_handle = ble_services[i].chrc[j].handle + 2;
				ble_services[i].chrc[j].subscribe_params.value_handle = ble_services[i].chrc[j].handle + 1;
				ble_services[i].chrc[j].subscribe_params.notify = notify_func;
				ble_services[i].chrc[j].subscribe_params.value = (ble_services[i].chrc[j].properties & BT_GATT_CHRC_NOTIFY) ?
						   BT_GATT_CCC_NOTIFY : BT_GATT_CCC_INDICATE;
				ble_services[i].chrc[j].subscribe_params.flags = BT_GATT_SUBSCRIBE_FLAG_VOLATILE;
				err = bt_gatt_subscribe(default_conn, &ble_services[i].chrc[j].subscribe_params);					
				if (err) {
					printk("subscribe failed (err %d)\n", err);
				} else {
					printk("subscribed\n");
				}
				ccc_cnt++;
				k_sleep(100);
			}
		}
	}

	printk("nofity_enable: ccc_cnt = %d\n", ccc_cnt);
	pair();
}

void show_chrc(struct ble_chrc *chrc) {
	char uuid[37] = {0};
	bt_uuid_to_str(&chrc->uuid, uuid, sizeof(uuid));
	printk("Charicteristic %s:  handle %x, properties %x\n", 
		uuid, chrc->handle, chrc->properties);
}

void show_services(void){
	int i, j;
	char uuid[37] = {0};

	for (i = 0; i < svc_cnt; i++) {
		bt_uuid_to_str(&ble_services[i].uuid, uuid, sizeof(uuid));
		printk("Service %s:  type %x start handle %x, end_handle %x chrc_cnt %x\n", 
			uuid, ble_services[i].type, ble_services[i].start_handle, ble_services[i].end_handle,  ble_services[i].chrc_cnt);
		for (j = 0; j < ble_services[i].chrc_cnt; j++) {
			show_chrc(&ble_services[i].chrc[j]);
		}
	}
}

u16_t uuid_len(const struct bt_uuid *uuid){
	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		return sizeof(struct bt_uuid_16);
	case BT_UUID_TYPE_32:
		return sizeof(struct bt_uuid_32);
	case BT_UUID_TYPE_128:
		return sizeof(struct bt_uuid_128);
	}
	return sizeof(struct bt_uuid_16);
}

void parse_service(const struct bt_gatt_service_val *gatt_service, 
	const struct bt_gatt_attr *attr, int type) {
	char uuid[37] = {0};

	bt_uuid_to_str(gatt_service->uuid, uuid, sizeof(uuid));
	printk("Service %s found: start handle %x, end_handle %x\n",
	   	    uuid, attr->handle, gatt_service->end_handle);
	
	if (svc_cnt >= SVC_MAX_CNT) {
		return;
	}	
	
	ble_services[svc_cnt].type = type;
	memcpy(&ble_services[svc_cnt].uuid, gatt_service->uuid, uuid_len(gatt_service->uuid));
	ble_services[svc_cnt].start_handle = attr->handle;
	ble_services[svc_cnt].end_handle = gatt_service->end_handle;

	svc_cnt++;
}

void parse_charicteristic(const struct bt_gatt_chrc *gatt_chrc, 
	const struct bt_gatt_attr *attr) 
{
	char uuid[37] = {0};
	int i, chrc_cnt;
	bt_uuid_to_str(gatt_chrc->uuid, uuid, sizeof(uuid));
	printk("Characteristic %s found: handle %x, properties %x\n", uuid,  attr->handle, gatt_chrc->properties);
	
 	for (i = 0; i < svc_cnt; i++) {
		if (attr->handle >= ble_services[i].start_handle && 
			attr->handle <= ble_services[i].end_handle) {
			break;		
		}
	}
	if (i == svc_cnt)
		return;
	
	chrc_cnt =  ble_services[i].chrc_cnt;
	if (chrc_cnt >= CHRC_MAX_CNT) {
		return;
	}
	memcpy(&ble_services[i].chrc[chrc_cnt].uuid, gatt_chrc->uuid, uuid_len(gatt_chrc->uuid));
	ble_services[i].chrc[chrc_cnt].handle = attr->handle;
	ble_services[i].chrc[chrc_cnt].properties = gatt_chrc->properties;
	ble_services[i].chrc_cnt++;	
}

static u8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_service_val *gatt_service;
	struct bt_gatt_chrc *gatt_chrc;

	if (!attr) {
		int err;
		if (params->type == BT_GATT_DISCOVER_PRIMARY) {
		    params->type = BT_GATT_DISCOVER_SECONDARY;
			printk("discover secondary service\n");
		} else if (params->type == BT_GATT_DISCOVER_SECONDARY) {
		    params->type = BT_GATT_DISCOVER_CHARACTERISTIC;		
			printk("discover charcteristic\n");
		} else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
			printk("Discover complete!\n");
			show_services();
			memset(params, 0, sizeof(*params));
			send_async_msg(MSG_DISCOVER_COMPLETE);
			return BT_GATT_ITER_STOP;		
		}
		
		discover_params.func = discover_func;
		discover_params.start_handle = 0x0001;
		discover_params.end_handle = 0xffff;
		err = bt_gatt_discover(default_conn, &discover_params);
		if (err)  {
			printk("Discover failed (err %d)\n", err);
		} else {
			printk("Discover pending\n");
		}
		return BT_GATT_ITER_STOP;		
	}

	switch (params->type) {
	case BT_GATT_DISCOVER_SECONDARY:
	case BT_GATT_DISCOVER_PRIMARY:
		gatt_service = attr->user_data;
		parse_service(gatt_service, attr, BT_GATT_DISCOVER_SECONDARY);
		break;
	case BT_GATT_DISCOVER_CHARACTERISTIC:
		gatt_chrc = attr->user_data;
		parse_charicteristic(gatt_chrc, attr);	
		break;
	default:
		break;
	}

	return BT_GATT_ITER_CONTINUE;
}

void start_discover(void)
{
	int err;
	if (!default_conn) {
		printk("Not connected\n");
		return;
	}
	
	discover_params.uuid = NULL;	
	discover_params.func = discover_func;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;
	err = bt_gatt_discover(default_conn, &discover_params);
	if (err)  {
		printk("Discover failed (err %d)\n", err);
	} else {
		printk("Discover pending\n");
	}
}

static int rmc_connect(const bt_addr_le_t *addr)
{
	struct bt_conn *conn_tmp;
	int err;
	
	err = bt_le_scan_stop();
	if (err) {
		printk("Stop LE scan failed (err %d)\n", err);
		return -1;
	}

	conn_tmp = bt_conn_create_le(addr, BT_LE_CONN_PARAM_DEFAULT);
	if (!conn_tmp) {
		printk("Connection failed\n");
		send_async_msg(MSG_SCAN);
		return -1;
	} else {
		printk("Connection pending\n");
		bt_conn_unref(conn_tmp);
	}
	
	return 0;
}

static bool rmc_found(u8_t type, const u8_t *data, u8_t data_len,
		      void *user_data)
{
	int err;
	bt_addr_le_t *addr = user_data;
	char name[30] = {0};

	switch (type) {
	case BT_DATA_NAME_SHORTENED: 
	case BT_DATA_NAME_COMPLETE:
		if (data_len > sizeof(name) - 1) {
			memcpy(name, data, sizeof(name) - 1);
		} else {
			memcpy(name, data, data_len);
		}
		
		printk("Device found name: %s\n", name);
		if (!strcmp(name, RMC_NAME)) {		
			printk("Remote Controllor found.\n");
			err = rmc_connect(addr);
			if (err) {
				printk("Failed to connect to remote controllor\n");
				break;
			}
			return true;
		}
		break;
	}

	return false;
}

static void adv_parse(struct net_buf_simple *ad,
		     bool (*func)(u8_t type, const u8_t *data,
				  u8_t data_len, void *user_data),
		     void *user_data)
{
	while (ad->len > 1) {
		u8_t len = net_buf_simple_pull_u8(ad);
		u8_t type;

		if (len == 0) {
			return;
		}

		if (len > ad->len || ad->len < 1) {
			printk("AD malformed\n");
			return;
		}

		type = net_buf_simple_pull_u8(ad);
		if (func(type, ad->data, len - 1, user_data)) {
			return;
		}

		net_buf_simple_pull(ad, len - 1);
	}
}

static void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
			 struct net_buf_simple *ad)
{
	int err;
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	printk("Device: %s, AD event type %u, AD data len %u, RSSI %i\n", dev, type, ad->len, rssi);

    /* connectable undirected event type and connectable directed event type */ 
	if (type == BT_LE_ADV_IND || type == BT_LE_ADV_SCAN_RSP) { 
		adv_parse(ad, rmc_found, (void *)addr);
	} else if (type == BT_LE_ADV_DIRECT_IND) {  // only passive scan
		printk("Directed advetisment\n");
		err = rmc_connect(addr);
		if (err) {
			printk("rmc_connect failed\n");
		}
	}
}

static void start_scan(int dups)
{
	int err;
	
	if (dups) {
		scan_param.filter_dup = dups;
	}

	printk("Start discover...\n");
	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Bluetooth set active scan failed (err %d)\n", err);
	} else {
		printk("Bluetooth active scan enabled\n");
	}	
}

static void connected(struct bt_conn *conn, u8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	
	if (conn_err) {
		printk("Failed to connect to %s (%x)\n", addr, conn_err);
		send_async_msg(MSG_SCAN);
		return;
	}
		
	default_conn = conn;
	bt_conn_ref(conn);

	printk("Connected: %s\n", addr);
	
	printk("send connected msg\n");
	send_async_msg(MSG_CONNECTED);
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	
	service_init();
	
	if (default_conn != conn) {
		printk("disconnected err\n");
		return;
	}	
	bt_conn_unref(default_conn);

	default_conn = NULL;
	printk("Disconnected: %s (reason %x)\n", addr, reason);
	
	printk("send disconnected msg\n");
	send_async_msg(MSG_DISCONNECTED);
}

static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (level != RMC_SECURITY_LEVEL){	
		if (security_err_cnt++ < RMC_SECURITY_TRY_MAX_TIMES) {
			int err;
			printk("Failed to set security, retry.\n");
			bt_keys_clear(bt_keys_find_addr(bt_conn_get_dst(conn)));
			err = bt_conn_security(default_conn, RMC_SECURITY_LEVEL);
			if (err) {
				printk("Failed to set security\n");
				security_err_cnt = 0;
				/* TODO: Handle security failure */
			}
		} else {
			printk("Security setting abort!\n");
			security_err_cnt = 0;
			/* TODO: Handle security failure */
		}
	} else {
		printk("Set security successfully, addr %s level %u \n", addr, level);
		security_err_cnt = 0;
	}
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	printk("LE conn param updated: int 0x%04x lat %d to %d\n", interval,
	       latency, timeout); 
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
	.le_param_updated = le_param_updated,
};

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	char passkey_str[7];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, 7, "%06u", passkey);

	printk("Confirm passkey for %s: %s\n", addr, passkey_str);

	bt_conn_auth_passkey_entry(conn, passkey);
}

static void auth_pairing_confirm(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Confirm pairing for %s\n", addr);

	bt_conn_auth_pairing_confirm(conn);
}

static struct bt_conn_auth_cb auth_callbacks = {
	.passkey_display = NULL,
	.passkey_entry = NULL,
	.passkey_confirm = auth_passkey_confirm,
	.cancel = NULL,
	.pairing_confirm = auth_pairing_confirm,
};

uint8_t bt_ready_flag;
__init_once_text static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	bt_ready_flag = 1;
}

void app_main(void)
{
	int err;
	struct app_msg msg;
	
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_callbacks);

	memset(&msg, 0, sizeof(msg));
	msg_manager_init();
	
	service_init();
	
	start_scan(DUP_DISABLE);
	
	while(1)
	{
		if(receive_msg(&msg, K_FOREVER))
		{
			switch(msg.type) {
			case MSG_CONNECTED:
				start_discover();
				break;
			case MSG_SCAN:
			case MSG_DISCONNECTED:
				start_scan(DUP_DISABLE);
				break;
			case MSG_DISCOVER_COMPLETE:
				nofity_enable();
				break;
			default:
				printk(" error message type msg.type %d \n",msg.type);
				continue;
			}
		}
	}
}

