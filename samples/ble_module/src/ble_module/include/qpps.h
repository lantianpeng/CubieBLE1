/**
 ****************************************************************************************
 *
 * @file qpps.h
 *
 * @brief Quintic Private Profile Server implementation.
 *
 * Copyright(C) 2015 NXP Semiconductors N.V.
 * All rights reserved.
 *
 ****************************************************************************************
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief  qpps service initilization
 *
 */

void qpps_init(void);

/** @brief send input report to peer device using qpps profile.
 *
 *
 *  @param conn  				: current ota connection.
 *  @param len 					: report data length.
 *  @param p_value 			:	Pointer to report data.
 */

int qpps_send_input_report(struct bt_conn *conn, u16_t len, u8_t *p_value);


#ifdef __cplusplus
}
#endif
