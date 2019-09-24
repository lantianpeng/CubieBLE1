/** @file
 *  @brief wp sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

void wp_init(void);
int wp_send_input_report(struct bt_conn *conn, u8_t reportId, u16_t len, u8_t *pValue);


#ifdef __cplusplus
}
#endif
