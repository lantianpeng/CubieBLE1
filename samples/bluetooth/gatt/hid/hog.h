/** @file
 *  @brief HoG Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

/* HID Spec Version: 1.11 */
#define HID_VERSION                   0x0111

/* HID Report Types */
#define HID_REPORT_TYPE_INPUT         0x01
#define HID_REPORT_TYPE_OUTPUT        0x02
#define HID_REPORT_TYPE_FEATURE       0x03

/* HID Protocol Mode Types */
#define HID_PROTOCOL_MODE_BOOT        0x00
#define HID_PROTOCOL_MODE_REPORT      0x01

/* HID Control Point Values */
#define HID_CONTROL_POINT_SUSPEND     0x00
#define HID_CONTROL_POINT_RESUME      0x01

/* Max length of an output report value */
#define HID_MAX_REPORT_LEN            32

/* Proprietary Service */
#define HID_START_HDL                 0x0
#define HID_END_HDL                   (HID_MAX_HDL - 1)

/* Proprietary Service Handles Common to HID Devices */
enum {
	HID_SVC_HDL = HID_START_HDL,        /* Proprietary Service Declaration */
	HID_INFO_CH_HDL,                    /* HID Information Characteristic Declaration */
	HID_INFO_HDL,                       /* HID Information Value */
	HID_REPORT_MAP_CH_HDL,              /* HID Report Map Characteristic Declaration */
	HID_REPORT_MAP_HDL,                 /* HID Report Map Value */
	HID_EXTERNAL_REPORT_HDL,            /* HID External Report Descriptor */
	HID_CONTROL_POINT_CH_HDL,           /* HID Control Point Characteristic Declaration */
	HID_CONTROL_POINT_HDL,              /* HID Control Point Value */
	HID_KEYBOARD_BOOT_IN_CH_HDL,        /* HID Keyboard Boot Input Characteristic Declaration */
	HID_KEYBOARD_BOOT_IN_HDL,           /* HID Keyboard Boot Input Value */
	HID_KEYBOARD_BOOT_IN_CH_CCC_HDL,    /* HID Keyboard Boot Input CCC Descriptor */
	HID_KEYBOARD_BOOT_OUT_CH_HDL,       /* HID Keyboard Boot Output Characteristic Declaration */
	HID_KEYBOARD_BOOT_OUT_HDL,          /* HID Keyboard Boot Output Value */
	HID_MOUSE_BOOT_IN_CH_HDL,           /* HID Mouse Boot Input Characteristic Declaration */
	HID_MOUSE_BOOT_IN_HDL,              /* HID Mouse Boot Input Value */
	HID_MOUSE_BOOT_IN_CH_CCC_HDL,       /* HID Mouse Boot Input CCC Descriptor */
	HID_INPUT_REPORT_1_CH_HDL,          /* HID Input Report Characteristic Declaration */
	HID_INPUT_REPORT_1_HDL,             /* HID Input Report Value */
	HID_INPUT_REPORT_1_CH_CCC_HDL,      /* HID Input Report CCC Descriptor */
	HID_INPUT_REPORT_1_REFERENCE_HDL,   /* HID Input Report Reference Descriptor */
	HID_INPUT_REPORT_2_CH_HDL,          /* HID Input Report Characteristic Declaration */
	HID_INPUT_REPORT_2_HDL,             /* HID Input Report Value */
	HID_INPUT_REPORT_2_CH_CCC_HDL,      /* HID Input Report CCC Descriptor */
	HID_INPUT_REPORT_2_REFERENCE_HDL,   /* HID Input Report Reference Descriptor */
	HID_INPUT_REPORT_3_CH_HDL,          /* HID Input Report Characteristic Declaration */
	HID_INPUT_REPORT_3_HDL,             /* HID Input Report Value */
	HID_INPUT_REPORT_3_CH_CCC_HDL,      /* HID Input Report CCC Descriptor */
	HID_INPUT_REPORT_3_REFERENCE_HDL,   /* HID Input Report Reference Descriptor */
	HID_OUTPUT_REPORT_CH_HDL,           /* HID Output Report Characteristic Declaration */
	HID_OUTPUT_REPORT_HDL,              /* HID Output Report Value */
	HID_OUTPUT_REPORT_REFERENCE_HDL,    /* HID Output Report Reference Descriptor */
	HID_FEATURE_REPORT_CH_HDL,          /* HID Feature Report Characteristic Declaration */
	HID_FEATURE_REPORT_HDL,             /* HID Feature Report Value */
	HID_FEATURE_REPORT_REFERENCE_HDL,   /* HID Feature Report Reference Descriptor */
	HID_PROTOCOL_MODE_CH_HDL,           /* HID Protocol Mode Characteristic Declaration */
	HID_PROTOCOL_MODE_HDL,              /* HID Protocol Mode Value */
	HID_MAX_HDL
};

/* Type of HID Information */
#define HID_INFO_CONTROL_POINT                  0
#define HID_INFO_PROTOCOL_MODE                  1

/* HID Boot Report ID */
#define HID_KEYBOARD_BOOT_ID                    0xFF
#define HID_MOUSE_BOOT_ID                       0xFE

typedef ssize_t (*hid_input_report_cback_t)(struct bt_conn *conn, u8_t id, u16_t len, u8_t *p_report);

typedef ssize_t (*hid_output_report_cback_t)(struct bt_conn *conn, u8_t id, u16_t len, u8_t *p_report);

typedef ssize_t (*hid_feature_report_cback_t)(struct bt_conn *conn, u8_t id, u16_t len, u8_t *p_report);

typedef ssize_t (*hid_info_cback_t)(struct bt_conn *conn, u8_t type, u8_t value);

/* HID Report Type/ID to Attribute handle map item */
struct hid_report_id_map_t {
	u8_t type;
	u8_t id;
	u16_t handle;
};

/* HID Profile Configuration */
struct hid_config_t {
	struct hid_report_id_map_t				*p_report_id_map;		/* A map between report Type/ID and Attribute handle */
	u8_t													report_id_map_size;	/* The number of Reports in the ID map (p_report_id_map) */
	hid_input_report_cback_t					input_cback;				/* Callback called on receipt of an Input Report */
	hid_output_report_cback_t				output_cback;			/* Callback called on receipt of an Output Report */
	hid_feature_report_cback_t 			feature_cback;			/* Callback called on receipt of a Feature Report */
	hid_info_cback_t								info_cback;				/* Callback called on receipt of protocol mode or control point */
	struct bt_gatt_ccc_cfg				*p_ccc_cfg;
	size_t			cfg_len;
	const u8_t *hid_report_map;
	u16_t hid_report_map_len;
};

/**
 *  @brief  hid profile initilization
 *
 */

void hog_init(void);

/** @brief send input report to peer device using hid profile.
 *
 *
 *  @param conn  				: current ota connection.
 *  @param report_id		: attr to be used to send data.
 *  @param len 					: report data length.
 *  @param p_value 			:	Pointer to report data.
 */

int hid_send_input_report(struct bt_conn *conn, u8_t report_id, u16_t len, u8_t *p_value);

/**
 *  @brief  set protocol mode into hog profile
 *
 *  @param  protocol_mode : protocol mode
 *
 */

void hid_set_protocol_mode(u8_t protocol_mode);

/**
 *  @brief  get protocol mode from hog profile
 *
 *  @return  protocol mode.
 */

u8_t hid_get_protocol_mode(void);

/**
 *  @brief  hid config initilization
 *
 *
 *
 */

void hid_init(const struct hid_config_t *p_config);

/**
 *  @brief  set ccc value to ccc table when ccc is changed
 *
 *
 *  @param  handle 	: ccc attr id
 *  @param  value 	: new ccc value
 *
 */

void hid_set_ccc_table_value(u16_t handle, u8_t value);


u8_t all_hid_ccc_is_enabled(void);

#ifdef __cplusplus
}
#endif
