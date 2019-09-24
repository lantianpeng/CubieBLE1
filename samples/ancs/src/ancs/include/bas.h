/** @file
 *  @brief BAS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*hw_read_batt_func_t)(u8_t *pLevel);

void bas_init(hw_read_batt_func_t func, struct bt_gatt_ccc_cfg	*ccc_cfg, size_t cfg_len);
void bas_notify(void);

#ifdef __cplusplus
}
#endif
