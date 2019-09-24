/* bt_config_init.c - bt host config handling */

/*
 * Copyright (c) 2017 Actions Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct cmd_data {
	/** BT_BUF_CMD */
	u8_t  type;

	/** HCI status of the command completion */
	u8_t  status;

	/** The command OpCode that the buffer contains */
	u16_t opcode;

	/** Used by bt_hci_cmd_send_sync. */
	struct k_sem *sync;
};

enum {
	ATT_PENDING_RSP,
	ATT_PENDING_CFM,
	ATT_DISCONNECTED,

	/* Total number of flags - must be at the end of the enum */
	ATT_NUM_FLAGS,
};

/* ATT channel specific context */
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

/* L2CAP signalling channel specific context */
struct bt_l2cap {
	/* The channel this context is associated with */
	struct bt_l2cap_le_chan	chan;
};
int bt_host_config_init(void);

extern int bt_init_transport(u16_t bt_mesh_adv_buf_count);
extern int bt_init_health(void);

extern int bt_l2cap_pool_init(struct bt_l2cap *pool, u16_t count);

extern int bt_smp_pool_init(struct bt_smp *pool, u16_t count);

extern int bt_smp_null_pool_init(struct bt_l2cap_le_chan *pool, u16_t count);

extern int bt_req_pool_int(struct bt_att *pool, int count);

extern int bt_prep_pool_int(struct net_buf_pool *pool, int count);

extern int bt_att_paramter_init(int att_mtu, int tx_limit);

extern int bt_tx_poll_event_init(struct k_poll_event *event);

extern int bt_gap_svc_config_init(char *name, u16_t len_name, u16_t appearance);

extern int bt_gatt_svc_config_init(struct bt_gatt_ccc_cfg *ccc_cfg, size_t ccc_cfg_size, void *func);

/* set l2cap_le_max_credits */
extern int bt_l2cap_le_max_credits_init(int count);

extern int bt_l2cap_pramter_init(int max_le_mps, int max_le_mtu);

extern int bt_hci_cmd_pool_init(struct net_buf_pool *pool);

extern int bt_hci_rx_pool_init(struct net_buf_pool **pool_array, int pool_num);

extern int bt_tx_thread_init(k_thread_stack_t stack, size_t stack_size, int prio);

extern int bt_rx_thread_init(k_thread_stack_t stack, size_t stack_size, int prio);

extern int bt_rpa_timeout_init(int timeout);

extern int bt_acl_tx_pool_init(struct net_buf_pool *pool);

extern int bt_conn_tx_init(struct bt_conn_tx *pconn_tx, int conn_tx_max);

extern int bt_conn_struct_init(struct bt_conn *conn, u16_t conn_num);

extern int bt_enable_private(int enable);

extern int bt_enable_smp(int enable);

extern int bt_host_keys_init(struct bt_keys *new_key_pool, int new_bt_max_paired);

extern int bt_set_delay_time_read_remote_features(int delay);

extern int (*pbt_host_config_init)(void);

extern void hci_vs_cfg_pwr_mgmt(u8_t power_mode, u32_t enable_op_flags, u32_t disable_op_flags);
extern void hci_vs_cfg_bd_addr(bt_addr_t *addr);
extern void hci_vs_cfg_acl_bufs(u8_t num_tx_bufs, u8_t num_rx_bufs, u16_t max_acl_len);
extern void hci_vs_cfg_version(u16_t comp_id, u16_t impl_rev, u8_t bt_ver);
extern void hci_vs_cfg_dev_filt(u8_t white_list_size, u8_t resolving_list_size);
extern void hci_vs_cfg_def_tx_pwr_lvl(int8_t def_tx_pwr_lvl);
extern void hci_vs_cfg_ce_jitter(u8_t ce_jitter_usec);
extern void hci_vs_cfg_max_conn(u8_t max_conn);
extern void hci_vs_cfg_dtm_rx_sync(u16_t dtm_rx_sync_ms);
extern void hci_vs_cfg_bufs(u8_t pool_idx, u8_t num_bufs, u16_t buf_size);
extern void hci_vs_cfg_max_scan_period(u16_t max_scan_period_ms);
extern void hci_vs_cfg_max_adv_reports(u8_t max_adv_reports);

extern void enable_scan_with_identity(int enable);

extern void hci_set_le_sup_feat(u8_t feat, u8_t flag);
