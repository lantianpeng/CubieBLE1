/* bt_config_init.c - bt host config handling */

/*Copyright (c) 2018 Actions (Zhuhai) Technology
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
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_driver.h>
#include <bluetooth/storage.h>
#include <bluetooth/att.h>
#include <bluetooth/gatt.h>

#include "common/log.h"

#include "common/rpa.h"
#include "keys.h"
#include "monitor.h"
#include "hci_core.h"
#include "hci_ecc.h"
#include "ecc.h"

#include "gatt_internal.h"
#include "att_internal.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "smp.h"
#include "crypto.h"

#include "bt_host_config_init.h"
#include "nvram_config.h"


#define CONFIG_BT_HCI_TX_PRIO 7
#define CONFIG_BT_RX_PRIO 8


#define L2CAP_LE_MAX_CREDITS		(CONFIG_BT_RX_BUF_COUNT - 1)

#define BT_L2CAP_RX_MTU (CONFIG_BT_RX_BUF_LEN - \
			 BT_HCI_ACL_HDR_SIZE - BT_L2CAP_HDR_SIZE)

#define CONFIG_BT_L2CAP_TX_USER_DATA_SIZE 4
#if BT_L2CAP_RX_MTU < CONFIG_BT_L2CAP_TX_MTU
#define BT_ATT_MTU BT_L2CAP_RX_MTU
#else
#define BT_ATT_MTU CONFIG_BT_L2CAP_TX_MTU
#endif

/* Size of MTU is based on the maximum amount of data the buffer can hold
 * excluding ACL and driver headers.
 */
#define L2CAP_MAX_LE_MPS	BT_L2CAP_RX_MTU
/* For now use MPS - SDU length to disable segmentation */
#define L2CAP_MAX_LE_MTU	(L2CAP_MAX_LE_MPS - 2)

#define RPA_TIMEOUT          K_SECONDS(CONFIG_BT_RPA_TIMEOUT)

/* command FIFO + conn_change signal + MAX_CONN * 2 (tx & tx_notify) */
#define EV_COUNT (2 + (CONFIG_BT_MAX_CONN * 2))

/*! /brief    Power management modes. */
enum {
	/*!< No power management. */
	BLE_TEST_POWER_MGMT_OFF				= 0,
	/*!< Disable clocks when not needed. */
	BLE_TEST_POWER_MGMT_CLOCK			= 1,
	/*!< Enter sleep. */
	BLE_TEST_POWER_MGMT_SLEEP			= 2,
	/*!< Enter deep sleep. */
	BLE_TEST_POWER_MGMT_DEEPSLEEP	= 3
};

/*! /brief      Operational flags. */
enum {
	/* Auto-restart Rx after CRC failure on an advertising PDU. */
	BLE_TEST_OP_FLAG_RX_AUTO_RESTART			= (1 << 0),
	/* Enable switcher during deep sleep. */
	BLE_TEST_OP_FLAG_ENA_SWITCHER_IN_SLEEP	= (1 << 1),
	/*!< Disable compensation for 32kHz timer. */
	BLE_TEST_OP_FLAG_DIS_32K_TICK_COMP			= (1 << 2),
	/*!< Enable HCI Tx acknowledgment. */
	BLE_TEST_OP_FLAG_ENA_HCI_TX_ACK				= (1 << 3)
};

#define COMP_ID_ACTIONS 0x03e0
#define IMPL_REV 0x114

enum {
	BT_4_0 = 0x6,
	BT_4_1 = 0x7,
	BT_4_2 = 0x8,
};


static char *gap_name = CONFIG_BT_DEVICE_NAME;
static u16_t gap_appearance = CONFIG_BT_DEVICE_APPEARANCE;

static struct bt_gatt_ccc_cfg sc_ccc_cfg[BT_GATT_CCC_MAX] = {};


static struct bt_conn_tx conn_tx[CONFIG_BT_CONN_TX_MAX];

static struct bt_conn conns[CONFIG_BT_MAX_CONN];

#define CMD_BUF_SIZE BT_BUF_RX_SIZE
NET_BUF_POOL_DEFINE(hci_cmd_pool, CONFIG_BT_HCI_CMD_COUNT,
		    CMD_BUF_SIZE, sizeof(struct cmd_data), NULL);

NET_BUF_POOL_DEFINE(hci_rx_pool_0, CONFIG_BT_RX_BUF_COUNT,
		    BT_BUF_RX_SIZE, BT_BUF_USER_DATA_MIN, NULL);

/*NET_BUF_POOL_DEFINE(hci_rx_pool_1, CONFIG_BT_RX_BUF_COUNT,
		    128, BT_BUF_USER_DATA_MIN, NULL);*/

NET_BUF_POOL_DEFINE(hci_rx_pool_2, CONFIG_BT_RX_BUF_COUNT,
		    64, BT_BUF_USER_DATA_MIN, NULL);

NET_BUF_POOL_DEFINE(hci_rx_pool_3, CONFIG_BT_RX_BUF_COUNT,
		    32, BT_BUF_USER_DATA_MIN, NULL);

static struct net_buf_pool *hci_rx_pool_array[] = {
	&hci_rx_pool_3,
	&hci_rx_pool_2,
	/*&hci_rx_pool_1,*/
	&hci_rx_pool_0,
};

NET_BUF_POOL_DEFINE(acl_tx_pool, CONFIG_BT_L2CAP_TX_BUF_COUNT,
		    BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_L2CAP_TX_USER_DATA_SIZE, NULL);

/* Pool for incoming ATT packets */
NET_BUF_POOL_DEFINE(prep_pool, CONFIG_BT_ATT_PREPARE_COUNT, BT_ATT_MTU,
		    sizeof(struct bt_attr_data), NULL);

static struct bt_att bt_req_pool[CONFIG_BT_MAX_CONN];

static struct bt_l2cap bt_l2cap_pool[CONFIG_BT_MAX_CONN];

static struct bt_smp bt_smp_pool[CONFIG_BT_MAX_CONN];
static struct bt_keys default_key_pool[CONFIG_BT_MAX_PAIRED];


static struct k_poll_event events[EV_COUNT] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&bt_dev.cmd_tx_queue,
					BT_EVENT_CMD_TX),
};

static BT_STACK_NOINIT(tx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static BT_STACK_NOINIT(rx_thread_stack, CONFIG_BT_RX_STACK_SIZE);

void rx_stack_analyze(void)
{
	STACK_ANALYZE_EXT("rx stack", rx_thread_stack,
		CONFIG_BT_RX_STACK_SIZE + BT_STACK_DEBUG_EXTRA);
}


static int char2hex(const char *c, u8_t *x)
{
	if (*c >= '0' && *c <= '9') {
		*x = *c - '0';
	} else if (*c >= 'a' && *c <= 'f') {
		*x = *c - 'a' + 10;
	} else if (*c >= 'A' && *c <= 'F') {
		*x = *c - 'A' + 10;
	} else {
		return -EINVAL;
	}

	return 0;
}

int str2bt_addr(const char *str, bt_addr_t *addr)
{
	int i, j;
	u8_t tmp;

	if (strlen(str) != 17) {
		return -EINVAL;
	}

	for (i = 5, j = 1; *str != '\0'; str++, j++) {
		if (!(j % 3) && (*str != ':')) {
			return -EINVAL;
		} else if (*str == ':') {
			i--;
			continue;
		}

		addr->val[i] = addr->val[i] << 4;

		if (char2hex(str, &tmp) < 0) {
			return -EINVAL;
		}

		addr->val[i] |= tmp;
	}

	return 0;
}

static void sc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
			       u16_t value)
{
	printk("ccc_cfg_value 0x%04x\n", value);
}

__init_once_text int bt_host_config_init(void)
{
	int err;

	/* config tx thread for bt host */
	err = bt_tx_thread_init(tx_thread_stack,
		K_THREAD_STACK_SIZEOF(tx_thread_stack), CONFIG_BT_HCI_TX_PRIO);
	if (err) {
		return err;
	}

	/* config rx thread for bt host */
	err = bt_rx_thread_init(rx_thread_stack,
		K_THREAD_STACK_SIZEOF(rx_thread_stack), CONFIG_BT_RX_PRIO);
	if (err) {
		return err;
	}

	/* config hci cmd buffer pool for bt host hci layer */
	err = bt_hci_cmd_pool_init(&hci_cmd_pool);
	if (err) {
		return err;
	}

	/* config hci rx buffer pool for bt host hci layer */
	err = bt_hci_rx_pool_init(hci_rx_pool_array, ARRAY_SIZE(hci_rx_pool_array));
	if (err) {
		return err;
	}

	/* config acl tx buffer pool for bt host hci layer */
	err = bt_acl_tx_pool_init(&acl_tx_pool);
	if (err) {
		return err;
	}

	/* config prepare buffer pool for bt host att layer */
	err = bt_prep_pool_int(&prep_pool, CONFIG_BT_ATT_PREPARE_COUNT);
	if (err) {
		return err;
	}

	/* config max credits paramter for bt host l2cap layer */
	err = bt_l2cap_le_max_credits_init(L2CAP_LE_MAX_CREDITS);
	if (err) {
		return err;
	}

	/* config mtu for bt host l2cap layer */
	err = bt_l2cap_pramter_init(L2CAP_MAX_LE_MPS, L2CAP_MAX_LE_MTU);
	if (err) {
		return err;
	}

	/* config max tx buffer simulately for bt host */
	err = bt_conn_tx_init(conn_tx, CONFIG_BT_CONN_TX_MAX);
	if (err) {
		return err;
	}

	/* config max supported conn for bt host */
	err = bt_conn_struct_init(conns, CONFIG_BT_MAX_CONN);
	if (err) {
		return err;
	}

	/* config att req buffer pool for bt host att layer */
	err = bt_req_pool_int(bt_req_pool, CONFIG_BT_MAX_CONN);
	if (err) {
		return err;
	}

	/* config l2cap pool for bt host l2cap layer */
	err = bt_l2cap_pool_init(bt_l2cap_pool, CONFIG_BT_MAX_CONN);
	if (err) {
		return err;
	}

	/* config tx pool events buffer for bt host */
	err = bt_tx_poll_event_init(events);
	if (err) {
		return err;
	}

	/* config mtu for bt host att layer */
	err = bt_att_paramter_init(BT_ATT_MTU, CONFIG_BT_ATT_TX_MAX);
	if (err) {
		return err;
	}

	/* config gap name for gap service */
	err = bt_gap_svc_config_init(gap_name, strlen(gap_name), gap_appearance);
	if (err) {
		return err;
	}

	/* config ccc callback for gatt service */
	err = bt_gatt_svc_config_init(sc_ccc_cfg, BT_GATT_CCC_MAX, sc_ccc_cfg_changed);
	if (err) {
		return err;
	}

	/* enable smp feature for bt host */
	err = bt_enable_smp(true);
	if (err) {
		return err;
	}

	/* config smp pool for bt host smp layer */
	err = bt_smp_pool_init(bt_smp_pool, CONFIG_BT_MAX_CONN);
	if (err) {
		return err;
	}

	/* config key pool for bt host smp layer */
	err = bt_host_keys_init(default_key_pool, CONFIG_BT_MAX_PAIRED);
	if (err) {
		return err;
	}

#if CONFIG_BT_PRIVACY
	/* enable private feature */
	err = bt_enable_private(true);
	if (err) {
		return err;
	}

	/* config timeout period for generating rpa addr */
	err = bt_rpa_timeout_init(RPA_TIMEOUT);
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_PRIVACY */

	/* config delay time: when connected, we read remote feature after delay time for compitable more mobile phone  */
	err = bt_set_delay_time_read_remote_features(K_SECONDS(1));
	if (err) {
		return err;
	}

	/* disable some ble feature for compitable more mobile phone */
	hci_set_le_sup_feat(BT_LE_FEAT_BIT_SLAVE_FEAT_REQ, 0);
	hci_set_le_sup_feat(BT_LE_FEAT_BIT_PHY_2M, 0);
	hci_set_le_sup_feat(BT_LE_FEAT_BIT_DLE, 0);
	hci_set_le_sup_feat(BT_LE_FEAT_BIT_PRIVACY, 0);

	/* Configure ACL buffers for ctrl*/
	hci_vs_cfg_acl_bufs(4, 4, 128);

	/* Configure maximum number of connections for ctrl */
	hci_vs_cfg_max_conn(CONFIG_BT_MAX_CONN);

	/* Configure version for ctrl */
	hci_vs_cfg_version(COMP_ID_ACTIONS, IMPL_REV, BT_4_2);

	/* Configure BD_ADDR for ctrl */
//#ifdef CONFIG_NVRAM_CONFIG
	char bt_addr_str[] = "21:22:33:44:55:66";
	bt_addr_t bt_addr;

//	err = nvram_config_get_factory("BT_ADDR", bt_addr_str, strlen(bt_addr_str));
//	if (err >= 0) {
		str2bt_addr(bt_addr_str, &bt_addr);
		hci_vs_cfg_bd_addr(&bt_addr);
//	}
//#endif

#ifdef CONFIG_BT_LLCC_SW_MODE
	/* enable ble sw mode */
	sys_write32(sys_read32(BLE_CTL) | (0x1<<BLE_CTL_BLE_SWV1V_REQ), BLE_CTL);

	/* Configure power manage mode for ctrl */
	hci_vs_cfg_pwr_mgmt(BLE_TEST_POWER_MGMT_DEEPSLEEP,
		BLE_TEST_OP_FLAG_ENA_HCI_TX_ACK | BLE_TEST_OP_FLAG_DIS_32K_TICK_COMP, BLE_TEST_OP_FLAG_RX_AUTO_RESTART);
#else
	/* Configure power manage mode for ctrl */
	hci_vs_cfg_pwr_mgmt(BLE_TEST_POWER_MGMT_DEEPSLEEP,
		BLE_TEST_OP_FLAG_ENA_HCI_TX_ACK | BLE_TEST_OP_FLAG_ENA_SWITCHER_IN_SLEEP |
		BLE_TEST_OP_FLAG_DIS_32K_TICK_COMP, BLE_TEST_OP_FLAG_RX_AUTO_RESTART);
#endif

	/* config scan with identity feature for bt host */
	enable_scan_with_identity(IS_ENABLED(CONFIG_BT_SCAN_WITH_IDENTITY));

	return 0;
}

static __init_once_text int _bt_host_config_init(struct device *unused)
{
	/* pbt_host_config_init will be called at bt_enable */
	pbt_host_config_init = bt_host_config_init;
	return 0;
}

SYS_INIT(_bt_host_config_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
