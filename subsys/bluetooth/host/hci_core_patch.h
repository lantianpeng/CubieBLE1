/*
 * Copyright (c) 2016 Intel Corporation
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
 */

#ifndef _HCI_CORE_PATCH_H_
#define _HCI_CORE_PATCH_H_

extern struct bt_le_conn_param *p_update_cfg;
extern int bt_conn_update_timeout;

extern void update_conn_param(struct bt_conn *conn);

extern void le_conn_update(struct k_work *work);

extern struct bt_att *att_get(struct bt_conn *conn);

int bt_set_conn_update_param(int idlePeriod,
			struct bt_le_conn_param *param);

int bt_le_direct_adv_start(bt_addr_le_t *peer_addr);

extern void att_req_sent(struct bt_conn *conn);

extern void att_rsp_sent(struct bt_conn *conn);

extern void att_cfm_sent(struct bt_conn *conn);

extern void smp_reset(void *smp);

extern void gatt_ccc_changed(const void *attr, void *ccc);

#if (CONFIG_BT_MAX_CONN == 1)

extern const struct bt_storage *bt_storage;
extern const struct bt_storage *bt_keys_storage;

void bt_gatt_load_ccc(void);

void bt_gatt_clear_ccc(void);

#else
/* TODO */
#endif

void rx_stack_analyze(void);

void hci_set_le_local_feat(u8_t feat, u8_t flag);

#endif

typedef bool (*app_ready_update_t)(void);

extern void update_conn_param(struct bt_conn *conn);

void register_app_ready_update_cback(app_ready_update_t app_ready_func);

