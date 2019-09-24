/** @file
 *  @brief Wireless Data Exchange service implementation sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OTA_PROFILE_H
#define OTA_PROFILE_H
#ifdef __cplusplus
extern "C" {
#endif


/* Proprietary Service */
#define OTA_START_HDL               0x0
#define OTA_END_HDL                 (OTA_MAX_HDL - 1)

/* Proprietary Service Handles */
enum {
	OTA_SVC_HDL = OTA_START_HDL,     /* Proprietary Service Declaration */
	OTA_DC_CH_HDL,                    /* WDX Device Configuration Characteristic Declaration */
	OTA_DC_HDL,                       /* WDX Device Configuration Characteristic Value */
	OTA_DC_CH_CCC_HDL,                /* WDX Device Configuration CCCD */
	OTA_FTC_CH_HDL,                   /* WDX File Transfer Control Characteristic Declaration */
	OTA_FTC_HDL,                      /* WDX File Transfer Control Characteristic Value */
	OTA_FTC_CH_CCC_HDL,               /* WDX File Transfer Control CCCD */
	OTA_FTD_CH_HDL,                   /* WDX File Transfer Data Characteristic Declaration */
	OTA_FTD_HDL,                      /* WDX File Transfer Data Characteristic Value */
	OTA_FTD_CH_CCC_HDL,               /* WDX File Transfer Data CCCD */
	OTA_AU_CH_HDL,                    /* WDX Authentication Characteristic Declaration */
	OTA_AU_HDL,                       /* WDX Authentication Characteristic Value */
	OTA_AU_CH_CCC_HDL,                /* WDX Authentication CCCD */
	OTA_MAX_HDL
};

typedef u8_t (*atts_write_cback_t)(struct bt_conn *conn, u16_t handle, u8_t operation,
						u16_t offset, u16_t len, u8_t *p_value,
						const struct bt_gatt_attr *attr);

/**
 *  @brief  ota service initilization
 *
 *
 *
 */

void ota_init(void);

/**
 *  @brief  register ota write cback into ota service
 *
 *
 *  @param  write_cback : write op will back to register module
 *
 */

void ota_register(atts_write_cback_t write_cback);

/**
 *  @brief  check whether attribute ccc is enabled
 *
 *
 *  @param  conn : current ota connection
 *  @param  handle: ccc attribute handle
 *
 *  @return  true if enabled or false.
 */

u8_t ota_ccc_enabled(struct bt_conn *conn, u8_t handle);

/** @brief Notify attribute value to peer device.
 *
 *
 *  @param conn  		: current ota connection.
 *  @param handle		: attr handle.
 *  @param len 			: Attribute value length.
 *  @param p_value 	:	Pointer to Attribute data.
 */
void ota_send_notify(struct bt_conn *conn, u8_t handle, u16_t len, u8_t *p_value);

#ifdef __cplusplus
}
#endif

#endif /* OTA_PROFILE_H */
