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
	
typedef void	(*wp_rx_notify_enabled_t)();

/**
 *  @brief  wp service initilization
 *
 */

void wp_init(void);

/** @brief send input report to peer device using wp profile.
 *
 *
 *  @param conn  				: current ota connection.
 *  @param report_id		: attr to be used to send data.
 *  @param len 					: report data length.
 *  @param p_value 			:	Pointer to report data.
 */

int wp_send_input_report(struct bt_conn *conn, u8_t report_id, u16_t len, u8_t *p_value);

/** @brief register notify enable callback.
 *
 *
 *  @param cb  				: wp_rx_notify_enabled_t callback to app.
 */
void wp_register_rx_notify_enabled_cb(wp_rx_notify_enabled_t cb);


#ifdef __cplusplus
}
#endif
