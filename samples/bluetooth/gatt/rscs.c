/** @file
 *  @brief RSCS Service sample
 */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <types_encode.h>

/** @def BT_UUID_RSCS
 *  @brief Running Speed and Cadence
 */
#define BT_UUID_RSCS                       BT_UUID_DECLARE_16(0x1814)
#define BT_UUID_RSCS_VAL                   0x1814
/** @def BT_UUID_RSC_MEASUREMENT
 *  @brief RSC Measurement
 */
#define BT_UUID_RSC_MEASUREMENT           BT_UUID_DECLARE_16(0x2a53)
#define BT_UUID_RSC_MEASUREMENT_VAL       0x2a53
/** @def BT_UUID_RSC_FEATURE
 *  @brief RSC Feature
 */
#define BT_UUID_RSC_FEATURE               BT_UUID_DECLARE_16(0x2a54)
#define BT_UUID_RSC_FEATURE_VAL           0x2a54
#if 0
/** @def BT_UUID_SENSOR_LOCATION
 *  @brief Sensor Location Characteristic
 */
#define BT_UUID_SENSOR_LOCATION           BT_UUID_DECLARE_16(0x2a5d)
#define BT_UUID_SENSOR_LOCATION_VAL       0x2a5d
/** @def BT_UUID_SC_CONTROL_POINT
 *  @brief SC Control Point Characteristic
 */
#define BT_UUID_SC_CONTROL_POINT          BT_UUID_DECLARE_16(0x2a55)
#define BT_UUID_SC_CONTROL_POINT_VAl      0x2a55
#endif

/* RSCS Application error codes */
#define RSCS_ERR_IN_PROGRESS	0x80
#define RSCS_ERR_CCC_CONFIG		0x81

/* SC Control Point Opcodes */
#define SC_CP_OP_SET_CWR		0x01
#define SC_CP_OP_CALIBRATION	0x02
#define SC_CP_OP_UPDATE_LOC		0x03
#define SC_CP_OP_REQ_SUPP_LOC	0x04
#define SC_CP_OP_RESPONSE		0x10

/* SC Control Point Response Values */
#define SC_CP_RSP_SUCCESS		0x01
#define SC_CP_RSP_OP_NOT_SUPP	0x02
#define SC_CP_RSP_INVAL_PARAM	0x03
#define SC_CP_RSP_FAILED		0x04

/* RSC Measurement Flags */
#define RSC_MEAS_FLAG_INSTANTANEOUS_STRIDE_LENGTH_PRESENT  BIT(0)
#define RSC_MEAS_FLAG_TOTAL_DISTANCE_PRESENT	           BIT(1)
#define RSC_MEAS_FLAG_WALKING_STATUS_BITS	   0
#define RSC_MEAS_FLAG_RUNNING_STATUS_BITS	   BIT(2)

/* RSC Sensor Locations */
#define RSC_SENSOR_LOCAL_OTHER			0x00
#define RSC_SENSOR_LOCAL_TOP_OF_SHOE	0x01
#define RSC_SENSOR_LOCAL_IN_SHOE		0x02
#define RSC_SENSOR_LOCAL_HIP			0x03
#define RSC_SENSOR_LOCAL_FRONT_WHEEL	0x04
#define RSC_SENSOR_LOCAL_LEFT_CRANK		0x05
#define RSC_SENSOR_LOCAL_RIGHT_CRANK	0x06
#define RSC_SENSOR_LOCAL_LEFT_PEDAL		0x07
#define RSC_SENSOR_LOCAL_RIGHT_PEDAL	0x08
#define RSC_SENSOR_LOCAL_FRONT_HUB		0x09
#define RSC_SENSOR_LOCAL_REAR_DROPOUT	0x0a
#define RSC_SENSOR_LOCAL_CHAINSTAY		0x0b
#define RSC_SENSOR_LOCAL_REAR_WHEEL		0x0c
#define RSC_SENSOR_LOCAL_REAR_HUB		0x0d
#define RSC_SENSOR_LOCAL_CHEST			0x0e
#define RSC_SENSOR_LOCAL_SPIDER			0x0f
#define RSC_SENSOR_LOCAL_CHAIN_RING		0x10

/* RSC Feature */
#define RSC_FEATURE_INSTANTANEOUS_STRIDE_LENGTH_MEASUREMENT_SUPPORTED  BIT(0)
#define RSC_FEATURE_TOTAL_DISTANCE_MEASUREMENT_SUPPORTED BIT(1)
#define RSC_FEATURE_WALKING_OR_RUNNING_STATUS_SUPPORTED	 BIT(2)
#define RSC_FEATURE_CALIBRATION_PROCEDURE_SUPPORTED	     BIT(3)
#define RSC_FEATURE_MULTI_SENSOR_LOCATIONS_SUPPORTED     BIT(4)

/* RSC Sensor Supported Location */
#define RCSC_SUPPORTED_LOCATIONS		{ RSC_SENSOR_LOCAL_OTHER, \
									  RSC_SENSOR_LOCAL_TOP_OF_SHOE, \
									  RSC_SENSOR_LOCAL_IN_SHOE, \
									  RSC_SENSOR_LOCAL_HIP, \
									  RSC_SENSOR_LOCAL_CHEST}
struct  rscs_rsc_measurement {
	u8_t flag;
	u16_t instantaneous_speed;             // Unit is in m/s with a resolution of 1/256 s
	u8_t instantaneous_cadence;            // Unit is in 1/minute (or RPM) with a resolutions of 1 1/min (or 1 RPM)
	u16_t instantaneous_stride_length;     // Unit is in meter with a resolution of 1/100 m (or centimeter).
	u32_t total_distance;                  // Unit is in meter with a resolution of 1/10 m (or decimeter).
} __packed;

static u32_t curr_total_distance;

struct bt_gatt_rscs {
	struct  rscs_rsc_measurement rsc_meas;
	u8_t rsc_meas_ccc;
	u16_t rsc_feature;
	u8_t sensor_location;
	u8_t sc_control_point_ccc;
} __packed;

static struct bt_gatt_rscs gatt_rscs;	
static struct bt_gatt_ccc_cfg rscs_rsc_meas_ccc_cfg[BT_GATT_CCC_MAX];
static struct bt_gatt_ccc_cfg rscs_sc_ctrl_point_ccc_cfg[BT_GATT_CCC_MAX];
static u8_t supported_locations[] = RCSC_SUPPORTED_LOCATIONS;
static struct k_timer rscs_timer;

static void rscs_rsc_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				     u16_t value)
{
	gatt_rscs.rsc_meas_ccc = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static void rscs_sc_ctrl_point_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				       u16_t value)
{
	gatt_rscs.sc_control_point_ccc = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

static ssize_t rscs_sensor_location_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     u16_t len, u16_t offset)
{
	u8_t *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(*value));
}

static ssize_t rscs_rsc_feature_read(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &gatt_rscs.rsc_feature, sizeof(gatt_rscs.rsc_feature));
}

struct write_sc_ctrl_point_req {
	u8_t op;
	union {
		u32_t total_distance;
		u8_t location;
	};
} __packed;

#define MAX_SC_CTRL_POINT_DATA_LEN 1
struct sc_ctrl_point_ind {
	u8_t op;
	u8_t req_op;
	u8_t status;
	u8_t data[MAX_SC_CTRL_POINT_DATA_LEN];
} __packed;

static struct bt_gatt_indicate_params ctrl_point_ind_params;
static struct sc_ctrl_point_ind ctrl_point_resp;
static u8_t ctrl_point_indicating;

static void ctrl_point_ind(struct bt_conn *conn, u8_t req_op, u8_t status,
			   const void *data, u16_t data_len);

static ssize_t rscs_sc_ctrl_point_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, u16_t len, u16_t offset,
				u8_t flags)
{
	const struct write_sc_ctrl_point_req *req = buf;
	u8_t status;
	int i;
	
	if (!gatt_rscs.sc_control_point_ccc) {
		return BT_GATT_ERR(RSCS_ERR_CCC_CONFIG);
	}

	if (!len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (ctrl_point_indicating) {
		return BT_GATT_ERR(RSCS_ERR_IN_PROGRESS);
	}

	switch (req->op) {
	case SC_CP_OP_SET_CWR:
		if (len != sizeof(req->op) + sizeof(req->total_distance)) {
			status = SC_CP_RSP_INVAL_PARAM;
			break;
		}
	
		gatt_rscs.rsc_meas.total_distance = sys_le32_to_cpu(req->total_distance);
		status = SC_CP_RSP_SUCCESS;
		break;
	case SC_CP_OP_UPDATE_LOC:
		if (len != sizeof(req->op) + sizeof(req->location)) {
			status = SC_CP_RSP_INVAL_PARAM;
			break;
		}

		/* Break if the requested location is the same as current one */
		if (req->location == gatt_rscs.sensor_location) {
			status = SC_CP_RSP_SUCCESS;
			break;
		}

		/* Pre-set status */
		status = SC_CP_RSP_INVAL_PARAM;

		/* Check if requested location is supported */
		for (i = 0; i < ARRAY_SIZE(supported_locations); i++) {
			if (supported_locations[i] == req->location) {
				gatt_rscs.sensor_location = req->location;
				status = SC_CP_RSP_SUCCESS;
				break;
			}
		}

		break;
	case SC_CP_OP_REQ_SUPP_LOC:
		if (len != sizeof(req->op)) {
			status = SC_CP_RSP_INVAL_PARAM;
			break;
		}

		/* Indicate supported locations and return */
		ctrl_point_ind(conn, req->op, SC_CP_RSP_SUCCESS,
			       &supported_locations,
			       sizeof(supported_locations));

		return len;
	default:
		status = SC_CP_RSP_OP_NOT_SUPP;
	}

	ctrl_point_ind(conn, req->op, status, NULL, 0);

	return len;
}

/* Running Speed and Cadence Service Declaration */
static struct bt_gatt_attr rscs_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_RSCS),

	BT_GATT_CHARACTERISTIC(BT_UUID_RSC_MEASUREMENT, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_RSC_MEASUREMENT, BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(rscs_rsc_meas_ccc_cfg, rscs_rsc_meas_ccc_cfg_changed),

	BT_GATT_CHARACTERISTIC(BT_UUID_SENSOR_LOCATION, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_SENSOR_LOCATION, BT_GATT_PERM_READ,
			   rscs_sensor_location_read, NULL, &gatt_rscs.sensor_location),

	BT_GATT_CHARACTERISTIC(BT_UUID_RSC_FEATURE, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_RSC_FEATURE, BT_GATT_PERM_READ,
			   rscs_rsc_feature_read, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_SC_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE),
	BT_GATT_DESCRIPTOR(BT_UUID_SC_CONTROL_POINT, BT_GATT_PERM_WRITE, NULL,
			   rscs_sc_ctrl_point_write, NULL),

	BT_GATT_CCC(rscs_sc_ctrl_point_ccc_cfg, rscs_sc_ctrl_point_ccc_cfg_changed),
};

static struct bt_gatt_service rscs_svc = BT_GATT_SERVICE(rscs_attrs);

static void ctrl_point_indicate_rsp(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, u8_t err)
{
	ctrl_point_indicating = 0;
	if (err) {
		printk("ctrl_point_indicate_rsp: err 0x%02x\n", err);
	}
}

static void ctrl_point_ind(struct bt_conn *conn, u8_t req_op, u8_t status,
			   const void *data, u16_t data_len)
{
	ctrl_point_resp.op = SC_CP_OP_RESPONSE;
	ctrl_point_resp.req_op = req_op;
	ctrl_point_resp.status = status;

	/* Send data (supported locations) if present */
	if (data && data_len) {
		memcpy(ctrl_point_resp.data, data, data_len);
	}

	ctrl_point_ind_params.attr = &rscs_attrs[9];
	ctrl_point_ind_params.func = ctrl_point_indicate_rsp;
	ctrl_point_ind_params.data = &ctrl_point_resp;
	ctrl_point_ind_params.len = sizeof(ctrl_point_resp) - sizeof(ctrl_point_resp.data) + data_len;

	printk(" send sc control points indicate\n");
	if (!bt_gatt_indicate(NULL, &ctrl_point_ind_params)) {
		ctrl_point_indicating = 1;
	}
}

static bool walking = false;

static void rsc_measurement_simulation(void)
{
	/* rsc measurement flags */
	gatt_rscs.rsc_meas.flag = RSC_MEAS_FLAG_INSTANTANEOUS_STRIDE_LENGTH_PRESENT
		| RSC_MEAS_FLAG_TOTAL_DISTANCE_PRESENT;
	
	if (walking) {
		gatt_rscs.rsc_meas.flag |= RSC_MEAS_FLAG_WALKING_STATUS_BITS;
	} else {
		gatt_rscs.rsc_meas.flag |= RSC_MEAS_FLAG_RUNNING_STATUS_BITS;
	}

	if (gatt_rscs.rsc_meas.flag & RSC_MEAS_FLAG_RUNNING_STATUS_BITS) {
		/* running */
		gatt_rscs.rsc_meas.instantaneous_speed = 256 * 4.5;  /* 5*3600=16200 */ 
		gatt_rscs.rsc_meas.instantaneous_cadence = 250;    /* 250*60*1.2=16200 */
		gatt_rscs.rsc_meas.instantaneous_stride_length = 100 * 1;   /* 1.08 */
		gatt_rscs.rsc_meas.total_distance += 50;
	} else {
		/* walking */
		gatt_rscs.rsc_meas.instantaneous_speed = 256 * 1.5;  /* 1.5*3600=5400 */ 
		gatt_rscs.rsc_meas.instantaneous_cadence = 110;   /* 110*60*0.75=5400 */
		gatt_rscs.rsc_meas.instantaneous_stride_length = 100 * 0.75;   /* 0.75 */
		gatt_rscs.rsc_meas.total_distance += 15;
	}
}

void rsc_measurement_notify(void)
{
	if (gatt_rscs.rsc_meas_ccc) {
		int err;
		int len = 0;
		u8_t buf[sizeof(gatt_rscs.rsc_meas)]  = {0};

		buf[len++] = gatt_rscs.rsc_meas.flag;
		len += u16_encode(gatt_rscs.rsc_meas.instantaneous_speed, &buf[len]);
		buf[len++] = gatt_rscs.rsc_meas.instantaneous_cadence;

		if (gatt_rscs.rsc_meas.flag & RSC_MEAS_FLAG_INSTANTANEOUS_STRIDE_LENGTH_PRESENT) {
			len += u16_encode(gatt_rscs.rsc_meas.instantaneous_stride_length, &buf[len]);
		}
		
		if (gatt_rscs.rsc_meas.flag & RSC_MEAS_FLAG_TOTAL_DISTANCE_PRESENT) {
			len += u32_encode(curr_total_distance, &buf[len]);
		}
		
		if (0 != (err = bt_gatt_notify(NULL, &rscs_attrs[2], buf, len))) {
			printk("rsc measurement notify failed (err %d)\n", err);
		}
	}
}

void rscs_timer_handler(struct k_timer *dummy)
{
	static int cnt = 0;
	
	/* rsc measurement */
	rsc_measurement_simulation();
	
	/* The Total Distance field, if supported, transmission approximately once 
	 * every 2 or 3 seconds. 
	 */
	if (cnt++ % 3 == 0) {
		/* refresh total distance */
		curr_total_distance = gatt_rscs.rsc_meas.total_distance;
	}
	
	/* The RSC Measurement characteristic is notified approximately once per second */
	rsc_measurement_notify();
}

void rscs_timer_init(void)
{
	k_timer_init(&rscs_timer, rscs_timer_handler, NULL);
	k_timer_start(&rscs_timer, K_SECONDS(1), K_SECONDS(1));
} 

void rscs_init(void)
{
	gatt_rscs.rsc_feature = RSC_FEATURE_INSTANTANEOUS_STRIDE_LENGTH_MEASUREMENT_SUPPORTED
		| RSC_FEATURE_TOTAL_DISTANCE_MEASUREMENT_SUPPORTED
		| RSC_FEATURE_WALKING_OR_RUNNING_STATUS_SUPPORTED
		| RSC_FEATURE_MULTI_SENSOR_LOCATIONS_SUPPORTED;
	gatt_rscs.sensor_location =  RSC_SENSOR_LOCAL_IN_SHOE;

	bt_gatt_service_register(&rscs_svc);

	rscs_timer_init();
}
