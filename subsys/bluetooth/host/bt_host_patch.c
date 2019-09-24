
/* hci_core.c - HCI core Bluetooth handling */

/*
 *Copyright (c) 2018 Actions (Zhuhai) Technology
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include "errno.h"
#include <atomic.h>
#include <misc/util.h>
#include <misc/slist.h>
#include <misc/stack.h>
#include <misc/__assert.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/gatt.h>

#include <conn_internal.h>
#include <l2cap_internal.h>

#include "hci_core.h"
#include "hci_core_patch.h"
#include <bluetooth/storage.h>
#include "keys.h"
#include <smp.h>
#include "nvram_config.h"

#define SYS_LOG_DOMAIN "bt_host"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

typedef bool (*app_ready_update_t)(void);
app_ready_update_t app_ready_update;
static u8_t update_delay_times;

void register_app_ready_update_cback(app_ready_update_t app_ready_func)
{
	app_ready_update = app_ready_func;
}


void update_conn_param_new(struct bt_conn *conn)
{
	/*
	 * Core 4.2 Vol 3, Part C, 9.3.12.2
	 * The Peripheral device should not perform a Connection Parameter
	 * Update procedure within 5 s after establishing a connection.
	 */
	k_delayed_work_submit(&conn->le.update_work,
				 conn->role == BT_HCI_ROLE_MASTER ? K_NO_WAIT :
				 bt_conn_update_timeout);
}

FUNCTION_PATCH_REGISTER(update_conn_param, update_conn_param_new, update_conn_param);

void le_conn_update_new(struct k_work *work)
{
	struct bt_conn_le *le = CONTAINER_OF(work, struct bt_conn_le,
					     update_work);
	struct bt_conn *conn = CONTAINER_OF(le, struct bt_conn, le);
	const struct bt_le_conn_param *param;

	if (conn->state == BT_CONN_CONNECT) {
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	if (app_ready_update && !app_ready_update() && (update_delay_times++ < 30)) {
		update_conn_param(conn);
		return;
	}

	update_delay_times = 0;

	if (p_update_cfg) {
		param = (const struct bt_le_conn_param *)p_update_cfg;
	} else {
		param = BT_LE_CONN_PARAM(conn->le.interval_min,
					 conn->le.interval_max,
					 conn->le.latency,
					 conn->le.timeout);
	}

	bt_conn_le_param_update(conn, param);
}

CODE_PATCH_REGISTER(le_conn_update, le_conn_update_new, 0x1859c);

enum {
	ATT_PENDING_RSP,
	ATT_PENDING_CFM,
	ATT_DISCONNECTED,

	/* Total number of flags - must be at the end of the enum */
	ATT_NUM_FLAGS,
};

struct bt_att {
	/* The channel this context is associated with */
	struct bt_l2cap_le_chan	chan;

	ATOMIC_DEFINE(flags, ATT_NUM_FLAGS);

	struct bt_att_req	*req;
	sys_slist_t		reqs;
	struct k_delayed_work	timeout_work;
	struct k_sem            tx_sem;
	struct k_fifo		prep_queue;
};

#define ATT_TIMEOUT				K_SECONDS(30)

void att_cfm_sent_new(struct bt_conn *conn)
{
	struct bt_att *att = att_get(conn);

	if (att == NULL)
		return;

	k_sem_give(&att->tx_sem);
}

FUNCTION_PATCH_REGISTER(att_cfm_sent, att_cfm_sent_new, att_cfm_sent);

void att_rsp_sent_new(struct bt_conn *conn)
{
	struct bt_att *att = att_get(conn);

	if (att == NULL)
		return;

	k_sem_give(&att->tx_sem);
}
FUNCTION_PATCH_REGISTER(att_rsp_sent, att_rsp_sent_new, att_rsp_sent);

void att_req_sent_new(struct bt_conn *conn)
{
	struct bt_att *att = att_get(conn);

	if (att == NULL)
		return;

	k_sem_give(&att->tx_sem);

	/* Start timeout work */
	if (att->req) {
		k_delayed_work_submit(&att->timeout_work, ATT_TIMEOUT);
	}
}

CODE_PATCH_REGISTER(att_req_sent, att_req_sent_new, 0x1af48);

/* patch for hciVsCb_cfgNext nop */
CODE_PATCH_REGISTER(hciVsCb_cfgNext_nop, 0x33008aa3, 0x17b84);

#if (CONFIG_BT_MAX_CONN == 1)

enum pairing_method {
	JUST_WORKS,		/* JustWorks pairing */
	PASSKEY_INPUT,		/* Passkey Entry input */
	PASSKEY_DISPLAY,	/* Passkey Entry display */
	PASSKEY_CONFIRM,	/* Passkey confirm */
	PASSKEY_ROLE,		/* Passkey Entry depends on role */
};

enum {
	SMP_FLAG_CFM_DELAYED,	/* if confirm should be send when TK is valid */
	SMP_FLAG_ENC_PENDING,	/* if waiting for an encryption change event */
	SMP_FLAG_KEYS_DISTR,	/* if keys distribution phase is in progress */
	SMP_FLAG_PAIRING,	/* if pairing is in progress */
	SMP_FLAG_TIMEOUT,	/* if SMP timeout occurred */
	SMP_FLAG_SC,		/* if LE Secure Connections is used */
	SMP_FLAG_PKEY_SEND,	/* if should send Public Key when available */
	SMP_FLAG_DHKEY_PENDING,	/* if waiting for local DHKey */
	SMP_FLAG_DHKEY_SEND,	/* if should generate and send DHKey Check */
	SMP_FLAG_USER,		/* if waiting for user input */
	SMP_FLAG_BOND,		/* if bonding */
	SMP_FLAG_SC_DEBUG_KEY,	/* if Secure Connection are using debug key */
	SMP_FLAG_SEC_REQ,	/* if Security Request was sent/received */
	SMP_FLAG_DHCHECK_WAIT,	/* if waiting for remote DHCheck (as slave) */
	SMP_FLAG_DERIVE_LK,	/* if Link Key should be derived */
	SMP_FLAG_BR_CONNECTED,	/* if BR/EDR channel is connected */
	SMP_FLAG_BR_PAIR,	/* if should start BR/EDR pairing */
	SMP_FLAG_CT2,		/* if should use H7 for keys derivation */

	/* Total number of flags - must be at the end */
	SMP_NUM_FLAGS,
};

/* SMP channel specific context */
struct bt_smp {
	/* The channel this context is associated with */
	struct bt_l2cap_le_chan	chan;

	/* Commands that remote is allowed to send */
	atomic_t		allowed_cmds;

	/* Flags for SMP state machine */
	ATOMIC_DEFINE(flags, SMP_NUM_FLAGS);

	/* Type of method used for pairing */
	u8_t			method;

	/* Pairing Request PDU */
	u8_t			preq[7];

	/* Pairing Response PDU */
	u8_t			prsp[7];

	/* Pairing Confirm PDU */
	u8_t			pcnf[16];

	/* Local random number */
	u8_t			prnd[16];

	/* Remote random number */
	u8_t			rrnd[16];

	/* Temporary key */
	u8_t			tk[16];

	/* Remote Public Key for LE SC */
	u8_t			pkey[64];

	/* DHKey */
	u8_t			dhkey[32];

	/* Remote DHKey check */
	u8_t			e[16];

	/* MacKey */
	u8_t			mackey[16];

	/* LE SC passkey */
	u32_t		passkey;

	/* LE SC passkey round */
	u8_t			passkey_round;

	/* Local key distribution */
	u8_t			local_dist;

	/* Remote key distribution */
	u8_t			remote_dist;

	/* Delayed work for timeout handling */
	struct k_delayed_work work;
};

void smp_reset_new(struct bt_smp *smp)
{
	struct bt_conn *conn = smp->chan.chan.conn;

	if (atomic_test_bit(smp->flags, SMP_FLAG_BOND)) {
		struct bt_keys *keys = smp->chan.chan.conn->le.keys;

		bt_storage->write(&keys->addr, BT_STORAGE_BT_KEYS, (void *)keys, sizeof(struct bt_keys));
	}

	k_delayed_work_cancel(&smp->work);

	smp->method = JUST_WORKS;
	atomic_set(&smp->allowed_cmds, 0);
	atomic_set(smp->flags, 0);

	if (conn->required_sec_level != conn->sec_level) {
		/* TODO report error */
		/* reset required security level in case of error */
		conn->required_sec_level = conn->sec_level;
	}

	if (conn->role == BT_HCI_ROLE_MASTER) {
		atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_SECURITY_REQUEST);
		return;
	}

	atomic_set_bit(&smp->allowed_cmds, BT_SMP_CMD_PAIRING_REQ);
}

FUNCTION_PATCH_REGISTER(smp_reset, smp_reset_new, smp_reset);

/* Persistent storage format for GATT CCC */
struct ccc_store {
	u16_t handle;
	u16_t value;
};

struct ccc_save {
	bt_addr_le_t addr;
	struct ccc_store store[CONFIG_CCC_STORE_MAX];
	size_t count;
};

static struct ccc_save ccc_save_storage;

static int ccc_store_find(u16_t handle)
{
	int i;

	for (i = 0; i < ccc_save_storage.count; i++) {
		if (ccc_save_storage.store[i].handle == handle)
			return i;
	}

	if (ccc_save_storage.count < CONFIG_CCC_STORE_MAX)
		return ccc_save_storage.count++;

	return -1;
}

void bt_gatt_store_ccc(bt_addr_le_t *addr, struct ccc_store *p_ccc_store)
{
	int i;

	if (bt_addr_le_cmp(addr, &ccc_save_storage.addr)) {
		memset(&ccc_save_storage, 0, sizeof(struct ccc_save));
		bt_addr_le_copy(&ccc_save_storage.addr, addr);
	}

	i = ccc_store_find(p_ccc_store->handle);
	if ((i >= 0) && (memcmp(&ccc_save_storage.store[i], p_ccc_store, sizeof(struct ccc_store)))) {
		memcpy(&ccc_save_storage.store[i], p_ccc_store, sizeof(struct ccc_store));
#ifdef CONFIG_NVRAM_CONFIG
		if (nvram_config_set("BT_CCC", &ccc_save_storage, sizeof(struct ccc_save)) >= 0)
			SYS_LOG_INF("Storing CCC: handle 0x%04x value 0x%04x", p_ccc_store->handle, p_ccc_store->value);
#endif
	}
}

static u8_t ccc_load(const struct bt_gatt_attr *attr, void *user_data)
{
	struct ccc_save *load = user_data;
	struct _bt_gatt_ccc *ccc;
	struct bt_gatt_ccc_cfg *cfg;
	int i;

	/* Check if attribute is a CCC */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	/* Clear if value was invalidade */
	if (!load->count) {
		return BT_GATT_ITER_STOP;
	}

	for (i = 0; i < load->count; i++) {
		if (load->store[i].handle == attr->handle) {
			ccc = attr->user_data;
			cfg = &ccc->cfg[0];
			bt_addr_le_copy(&cfg->peer, &load->addr);
			cfg->value = load->store[i].value;
			ccc->value = cfg->value;
			ccc->cfg_changed(attr, ccc->value);
			SYS_LOG_INF("Restoring CCC: handle 0x%04x value 0x%04x", load->store[i].handle, load->store[i].value);
			break;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

void bt_gatt_load_ccc(void)
{
	ssize_t ret = 0;
#ifdef CONFIG_NVRAM_CONFIG
	ret = nvram_config_get("BT_CCC", &ccc_save_storage, sizeof(struct ccc_save));
#endif
	if (ret >= 0)
		bt_gatt_foreach_attr(0x0001, 0xffff, ccc_load, &ccc_save_storage);
	else {
		SYS_LOG_INF("bt_gatt_load_ccc failed\n");
	}
}

static u8_t ccc_clear(const struct bt_gatt_attr *attr, void *user_data)
{
	struct _bt_gatt_ccc *ccc;
	struct bt_gatt_ccc_cfg *cfg;

	/* Check if attribute is a CCC */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;
	cfg = &ccc->cfg[0];

	/* Reset value */
	if (ccc->cfg_changed && (ccc->value || cfg->value)) {
		ccc->value = 0;
		cfg->value = 0;
		ccc->cfg_changed(attr, 0);
		SYS_LOG_INF("reset CCC: handle 0x%04x value 0x%04x", attr->handle, ccc->value);
	}

	return BT_GATT_ITER_CONTINUE;
}

void bt_gatt_clear_ccc(void)
{
#ifdef CONFIG_NVRAM_CONFIG
	memset(&ccc_save_storage, 0, sizeof(struct ccc_save));
	nvram_config_set("BT_CCC", &ccc_save_storage, sizeof(struct ccc_save));
#endif

	bt_gatt_foreach_attr(0x0001, 0xffff, ccc_clear, NULL);
	SYS_LOG_INF("bt_gatt_clear_ccc");
}

void gatt_ccc_changed_new(const struct bt_gatt_attr *attr, struct _bt_gatt_ccc *ccc)
{
	int i;
	u16_t value = 0x0000;

	for (i = 0; i < ccc->cfg_len; i++) {
		if (ccc->cfg[i].value > value) {
			value = ccc->cfg[i].value;
		}
	}

	if (value != ccc->value) {
		ccc->value = value;
		ccc->cfg_changed(attr, value);
		if (bt_addr_le_is_bonded(&ccc->cfg[0].peer)) {
			struct ccc_store ccc_store_tmp;

			ccc_store_tmp.handle = attr->handle;
			ccc_store_tmp.value = value;
			bt_gatt_store_ccc(&ccc->cfg[0].peer, &ccc_store_tmp);
		}
	}
}

FUNCTION_PATCH_REGISTER(gatt_ccc_changed, gatt_ccc_changed_new, gatt_ccc_changed);

/* patch for ccc_cfg valid */
CODE_PATCH_REGISTER(ccc_cfg_valid, 0x2300d003, 0x1c9d8);

#else
/* TODO */
#endif

/* test vendor specific OCF range is 0x000-0x00F. */
#define LE_EXT_OCF_BASE                   0x3E0
#define LE_EXT_OCF_END                    0x3FF

#define LE_EXT_OPCODE_VS_SET_LOCAL_FEAT          BT_OP(BT_OGF_VS, LE_EXT_OCF_BASE + 0x12)  /*!< Set Local Feature opcode. */
typedef struct {
	u8_t  val[8];
} local_feat_t;

int hci_vs_set_local_feat(local_feat_t *feat)
{
	int err;
	struct net_buf *buf;
	u8_t *p;

	buf = bt_hci_cmd_create(LE_EXT_OPCODE_VS_SET_LOCAL_FEAT,
				sizeof(local_feat_t));
	if (!buf) {
		return -ENOBUFS;
	}

	p = net_buf_add(buf, sizeof(local_feat_t));

	memcpy(p, feat, sizeof(local_feat_t));

	err = bt_hci_cmd_send_sync(LE_EXT_OPCODE_VS_SET_LOCAL_FEAT, buf,
				   NULL);
	if (err) {
		SYS_LOG_ERR("set local feature failed %d\n", err);
		return err;
	}
	return 0;
}

void hci_set_le_local_feat(u8_t feat, u8_t flag)
{
	struct net_buf *rsp;
	int err;
	local_feat_t feature;

	/* Read Low Energy Supported Features */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_LOCAL_FEATURES, NULL,
				 &rsp);
	if (err) {
		SYS_LOG_INF("read le feature failed\n");
		return;
	} else {
		struct bt_hci_rp_le_read_local_features *rp = (void *)rsp->data;

		if (rp->status)
			return;
		memcpy(&feature, rp->features, sizeof(local_feat_t));
	}

	if (flag) {
		/* set feature bit */
		feature.val[(feat) >> 3] |= BIT((feat) & 7);
	} else {
		/* clear feature bit */
		feature.val[(feat) >> 3] &= ~BIT((feat) & 7);
	}

	hci_vs_set_local_feat(&feature);

	net_buf_unref(rsp);
}

