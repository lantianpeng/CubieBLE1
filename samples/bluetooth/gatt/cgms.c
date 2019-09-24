/** @file
 *  @brief CGMS Service sample
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
#include "cgms.h"
#include "cgms_db.h"

void invert_u8(u8_t *dst_buf,u8_t *src_buf)  
{  
	int i;  
	u8_t tmp[4];  

	tmp[0] = 0;  
	for(i = 0; i < 8; i++) {  
		if(src_buf[0] & (1 << i)) {
			tmp[0] |= 1 << (7 - i);  
		}
	}  
	dst_buf[0] = tmp[0]; 
}  

void invert_u16(u16_t *dst_buf,u16_t *src_buf)  
{  
	int i;  
	u16_t tmp[4]; 
	
	tmp[0] = 0;  
	for(i = 0; i < 16; i++) {  
		if(src_buf[0] & (1 << i)) {
			tmp[0] |= 1 << (15 - i);  
		}
	}  
	dst_buf[0] = tmp[0];  
}  

static u16_t crc16_ccitt(u8_t *push_msg, u32_t len)  
{  
	u16_t crcin = 0xffff;  
	u16_t cpoly = 0x1021;  
	u8_t c = 0; 
	int i;

	while (len--) {  
		c = *(push_msg++);  
		invert_u8(&c, &c);  
		crcin ^= (c << 8);  
		for(i = 0; i < 8; i++) {  
			if(crcin & 0x8000) {
				crcin = (crcin << 1) ^ cpoly;  
			} else {
				crcin = crcin << 1;  
			}
		}  
	}  
	
	invert_u16(&crcin, &crcin);  
	
	return (crcin) ;  
}  

/* CGMS Application error codes */
#define CGMS_ERR_MISSING_CRC		0x80
#define CGMS_ERR_INVALID_CRC		0x81

/* CGM measurement Flags */
#define CGM_TREND_INFORMATION_PRESENT    (1 << 0)
#define CGM_QUALITY_PRESENT	             (1 << 1)
#define CGM_SENSOR_STAT_ANNU_WARNING_OCTET_PRESENT	      (1 << 5)
#define CGM_SENSOR_STAT_ANNU_CAL_OR_TEMP_OCTET_PRESENT    (1 << 6)
#define CGM_SENSOR_STAT_ANNU_STAT_OCTET_PRESENT	          (1 << 7)

/* CGM measurement Sensor Status Annunciation Field */
#define CGM_MEAS_SESSION_STOPPED	      (1 << 0)
#define CGM_MEAS_DEVICE_BATTERY_LOW	      (1 << 1)
#define CGM_MEAS_TYPE_INCORRECT           (1 << 2)
#define CGM_MEAS_SENSOR_MALFUNCTION	      (1 << 3)
#define CGM_MEAS_DEVICE_SPECIFIC_ALERT	  (1 << 4)
#define CGM_MEAS_GENERAL_DEVICE_FAULT     (1 << 5)
#define CGM_MEAS_TIME_SYNCHRONIZATION_REQUIRED	 (1 << 8)
#define CGM_MEAS_CALIBRATION_NOT_ALLOWED	     (1 << 9)
#define CGM_MEAS_CALIBRATION_RECOMMENDED	     (1 << 10)
#define CGM_MEAS_CALIBRATION_REQUIRED	         (1 << 11)
#define CGM_MEAS_TEMPERATURE_TOO_HIGH            (1 << 12)
#define CGM_MEAS_TEMPERATURE_TOO_LOW	         (1 << 13)
#define CGM_MEAS_RESULT_LOWER_THAN_LOW_LEVEL	 (1 << 16)
#define CGM_MEAS_RESULT_HIGHER_THAN_HIGH_LEVEL	 (1 << 17)
#define CGM_MEAS_RESULT_LOWER_THAN_HYPO_LEVEL	 (1 << 18)
#define CGM_MEAS_RESULT_HIGHER_THAN_HYPER_LEVEL	 (1 << 19)
#define CGM_MEAS_RATE_OF_DECREASE_EXCEEDED	     (1 << 20)
#define CGM_MEAS_RATE_OF_INCREASE_EXCEEDED	     (1 << 21)
#define CGM_MEAS_RESULT_LOWER_THAN_CAN_PROCESS	 (1 << 22)
#define CGM_MEAS_RESULT_HIGHER_THAN_CAN_PROCESS	 (1 << 23)

/* CGM Feature */
#define CGM_FEAT_CALIBRATION_SUPPORTED	                         (1 << 0)       
#define CGM_FEAT_PATIENT_HIGH_LOW_ALERTS_SUPPORTED	             (1 << 1)
#define CGM_FEAT_HYPO_ALERTS_SUPPORTED	                         (1 << 2)
#define CGM_FEAT_HYPER_ALERTS_SUPPORTED	                         (1 << 3)
#define CGM_FEAT_RATE_OF_INCREASE_DECREASE_ALERTS_SUPPORTED	     (1 << 4)
#define CGM_FEAT_DEVICE_SPECIFIC_ALERT_SUPPORTED	             (1 << 5)
#define CGM_FEAT_SENSOR_MALFUNCTION_DETECTION_SUPPORTED	         (1 << 6)
#define CGM_FEAT_SENSOR_TEMPERATURE_HIGH_LOW_DETECTION_SUPPORTED (1 << 7)
#define CGM_FEAT_SENSOR_RESULT_HIGH_LOW_DETECTION_SUPPORTED	     (1 << 8)
#define CGM_FEAT_LOW_BATTERY_DETECTION_SUPPORTED	             (1 << 9)
#define CGM_FEAT_SENSOR_TYPE_ERROR_DETECTION_SUPPORTED	         (1 << 10)
#define CGM_FEAT_GENERAL_DEVICE_FAULT_SUPPORTED	                 (1 << 11)
#define CGM_FEAT_E2E_CRC_SUPPORTED	                             (1 << 12)
#define CGM_FEAT_MULTIPLE_BOND_SUPPORTED	                     (1 << 13)
#define CGM_FEAT_MULTIPLE_SESSIONS_SUPPORTED	                 (1 << 14)
#define CGM_FEAT_CGM_TREND_INFORMATION_SUPPORTED	             (1 << 15)
#define CGM_FEAT_CGM_QUALITY_SUPPORTED	                         (1 << 16)

/* CGM Type */
#define CGM_TYPE_CAPILLARY_WHOLE_BLOOD    1
#define CGM_TYPE_CAPILLARY_PLASMA         2
#define CGM_TYPE_CAPILLARY_WHOLE_BLOOD2   3    // the same to bit 1 ??
#define CGM_TYPE_VENOUS_PLASMA            4
#define CGM_TYPE_ARTERIAL_WHOLE_BLOOD     5
#define CGM_TYPE_ARTERIAL_PLASMA          6
#define CGM_TYPE_UNDETERMINED_WHOLE_BLOOD 7
#define CGM_TYPE_UNDETERMINED_PLASMA      8
#define CGM_TYPE_INTERSTITIAL_FLUID       9
#define CGM_TYPE_CONTROL_SOLUTION         10

/* CGM Sample Location */
#define CGM_SAMPLE_LOCAL_FINGER              1
#define CGM_SAMPLE_LOCAL_ALTERNATE_SITE_TEST 2
#define CGM_SAMPLE_LOCAL_EARLOBE             3
#define CGM_SAMPLE_LOCAL_CONTROL_SOLUTION    4
#define CGM_SAMPLE_LOCAL_SUBCUTANEOUS_TISSUE 5
#define CGM_SAMPLE_LOCAL_NOT_AVAILABLE       15

/* DST offset value */
#define DST_OFFSET_STANDARD_TIME                0
#define DST_OFFSET_HALF_AN_HOUR_DAYLIGHT_TIME   2     // +0.5h
#define DST_OFFSET_DAYLIGHT_TIME                4     // +1h
#define DST_OFFSET_DOUBLE_DAYLIGHT_TIME         8     // +2h
#define DST_OFFSET_UNKNOW                       0xff

#define CGM_MEAS_MAX_SIZE 18
#define CGM_STATUS_MAX_SIZE 7

/* CGM Specific Ops Control Point Op Code  */
#define SOCP_OPCODE_RESERVED_FOR_FUTURE_USE                0
#define SOCP_OPCODE_SET_CGM_COMMUNICATION_INTERVAL         1
#define SOCP_OPCODE_GET_CGM_COMMUNICATION_INTERVAL         2
#define SOCP_OPCODE_CGM_COMMUNICATION_INTERVAL_RESPONSE    3
#define SOCP_OPCODE_SET_GLUCOSE_CALIBRATION_VALUE          4
#define SOCP_OPCODE_GET_GLUCOSE_CALIBRATION_VALUE          5
#define SOCP_OPCODE_GLUCOSE_CALIBRATION_VALUE_RESPONSE     6
#define SOCP_OPCODE_SET_PATIENT_HIGH_ALERT_LEVEL           7
#define SOCP_OPCODE_GET_PATIENT_HIGH_ALERT_LEVEL           8
#define SOCP_OPCODE_PATIENT_HIGH_ALERT_LEVEL_RESPONSE      9
#define SOCP_OPCODE_SET_PATIENT_LOW_ALERT_LEVEL            10
#define SOCP_OPCODE_GET_PATIENT_LOW_ALERT_LEVEL            11
#define SOCP_OPCODE_PATIENT_LOW_ALERT_LEVEL_RESPONSE       12
#define SOCP_OPCODE_SET_HYPO_ALERT_LEVEL                   13
#define SOCP_OPCODE_GET_HYPO_ALERT_LEVEL                   14
#define SOCP_OPCODE_HYPO_ALERT_LEVEL_RESPONSE              15
#define SOCP_OPCODE_SET_HYPER_ALERT_LEVEL                  16
#define SOCP_OPCODE_GET_HYPER_ALERT_LEVEL                  17
#define SOCP_OPCODE_HYPER_ALERT_LEVEL_RESPONSE             18
#define SOCP_OPCODE_SET_RATE_OF_DECREASE_ALERT_LEVEL       19
#define SOCP_OPCODE_GET_RATE_OF_DECREASE_ALERT_LEVEL       20
#define SOCP_OPCODE_RATE_OF_DECREASE_ALERT_LEVEL_RESPONSE  21
#define SOCP_OPCODE_SET_RATE_OF_INCREASE_ALERT_LEVEL       22
#define SOCP_OPCODE_GET_RATE_OF_INCREASE_ALERT_LEVEL       23
#define SOCP_OPCODE_RATE_OF_INCREASE_ALERT_LEVEL_RESPONSE  24
#define SOCP_OPCODE_RESET_DEVICE_SPECIFIC_ALERT            25
#define SOCP_OPCODE_START_THE_SESSION                      26
#define SOCP_OPCODE_STOP_THE_SESSION                       27
#define SOCP_OPCODE_RESPONSE_CODE                          28

/* CGM Specific Ops Control Point Op Code - Response Codes  */
#define SOCP_RESPONSE_RESERVED                0
#define SOCP_RESPONSE_SUCCESS                 1
#define SOCP_RESPONSE_OPCODE_UNSUPPORTED      2
#define SOCP_RESPONSE_INVALID_OPERAND         3
#define SOCP_RESPONSE_PROCEDURE_NOT_COMPLETED 4
#define SOCP_RESPONSE_PARAM_OUT_OF_RANGE      5

/* CGM Specific Ops Control Point Operand  */
#define SOCP_OPERAND_RESERVED_FOR_FUTURE_USE                 0
#define SOCP_OPERAND_COMMUNICATION_INTERVAL_IN_MINUTES       1
#define SOCP_OPERAND_COMMUNICATION_INTERVAL_IN_MINUTES2      3
#define SOCP_OPERAND_OPERAND_DEFINED_IN_CALIBRATION          4
#define SOCP_OPERAND_CALIBRATION_DATA_RECORD_NUMBER          5
#define SOCP_OPERAND_CALIBRATION_DATA                        6
#define SOCP_OPERAND_PATIENT_HIGH_BG_IN_MG_DL                7
#define SOCP_OPERAND_PATIENT_HIGH_BG_IN_MG_DL2               9
#define SOCP_OPERAND_PATIENT_LOW_BG_IN_MG_DL                 10
#define SOCP_OPERAND_PATIENT_LOW_BG_IN_MG_DL2                12
#define SOCP_OPERAND_HYPO_ALERT_LEVEL_IN_MG_DL               13
#define SOCP_OPERAND_HYPO_ALERT_LEVEL_IN_MG_DL2              15
#define SOCP_OPERAND_HYPER_ALERT_LEVEL_IN_MG_DL              16
#define SOCP_OPERAND_HYPER_ALERT_LEVEL_IN_MG_DL2             18
#define SOCP_OPERAND_RATE_OF_DEC_ALERT_LEVEL_IN_MG_DL_MIN    19
#define SOCP_OPERAND_RATE_OF_DEC_ALERT_LEVEL_IN_MG_DL_MIN2   21
#define SOCP_OPERAND_RATE_OF_INC_ALERT_LEVEL_IN_MG_DL_MIN    22
#define SOCP_OPERAND_RATE_OF_INC_ALERT_LEVEL_IN_MG_DL_MIN2   24
#define SOCP_OPERAND_REQUEST_OP_CODE_RESPONSE_CODE_VALUE     28

/* CGM Specific Ops Control Point Calibration Value - Calibration Status  */
#define SOCP_CALIBRATION_STATUS_CALIBRATION_DATA_REJECTED     (0 << 1) 
#define SOCP_CALIBRATION_STATUS_CALIBRATION_DATA_OUT_OF_RANGE (1 << 1) 	
#define SOCP_CALIBRATION_STATUS_CALIBRATION_PROCESS_PENDING   (2 << 1)

static struct bt_gatt_cgms gatt_cgms;	
static struct bt_gatt_ccc_cfg cgms_cgm_meas_ccc_cfg[BT_GATT_CCC_MAX];
static struct bt_gatt_ccc_cfg cgms_racp_ccc_cfg[BT_GATT_CCC_MAX];
static struct bt_gatt_ccc_cfg cgms_cgm_spec_ops_ctrl_point_ccc_cfg[BT_GATT_CCC_MAX];

static u8_t communication_interval = 0;// uint in minutes

static ssize_t cgms_racp_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, u16_t len, u16_t offset,
				u8_t flags);

static void cgms_cgm_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				     u16_t value)
{
	gatt_cgms.cgm_meas_ccc = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
	printk("cgm_meas_ccc=%d\n", gatt_cgms.cgm_meas_ccc);
}

static void cgms_racp_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				       u16_t value)
{
	gatt_cgms.racp_ccc = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
	printk("racp_ccc=%d\n", gatt_cgms.racp_ccc);
}

static void cgms_cgm_spec_ops_ctrl_point_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				       u16_t value)
{
	gatt_cgms.socp_ccc = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
	printk("socp_ccc=%d\n", gatt_cgms.socp_ccc);
}

static ssize_t cgms_cgm_feature_read(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				u16_t len, u16_t offset)
{
	u8_t feature_buf[6] = {0};
	int feature_len = 0;
	
	feature_len += u24_encode(gatt_cgms.cgm_feature.cgm_feature, &feature_buf[feature_len]);
	feature_buf[feature_len++] = gatt_cgms.cgm_feature.cgm_type_sample_location;
	feature_len += u16_encode(gatt_cgms.cgm_status.e2e_crc, &feature_buf[feature_len]);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
			 feature_buf, feature_len);	
}

static ssize_t cgms_cgm_status_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     u16_t len, u16_t offset)
{
	u8_t status_buf[CGM_STATUS_MAX_SIZE] = {0};
	int status_len = 0;

	status_len += u16_encode(gatt_cgms.cgm_status.time_offset, &status_buf[status_len]);
	status_len += u24_encode(gatt_cgms.cgm_status.sensor_status, &status_buf[status_len]);
	
	if (gatt_cgms.cgm_feature.cgm_feature & CGM_FEAT_E2E_CRC_SUPPORTED) {
		status_len += u16_encode(gatt_cgms.cgm_status.e2e_crc, &status_buf[status_len]);
	}
	
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 status_buf, status_len);
}

static ssize_t cgms_cgm_start_time_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     u16_t len, u16_t offset)
{
	int size = sizeof(gatt_cgms.cgm_session_start_time) - sizeof(u16_t);
	
	if (gatt_cgms.cgm_feature.cgm_feature & CGM_FEAT_E2E_CRC_SUPPORTED) {
		size += sizeof(u16_t);
	}
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
			 &gatt_cgms.cgm_session_start_time, size);
}

static ssize_t cgms_cgm_run_time_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     u16_t len, u16_t offset)
{
	int size = sizeof(gatt_cgms.cgm_session_run_time) - sizeof(u16_t);
	
	if (gatt_cgms.cgm_feature.cgm_feature & CGM_FEAT_E2E_CRC_SUPPORTED) {
		gatt_cgms.cgm_session_run_time.e2e_crc = crc16_ccitt((u8_t *)&gatt_cgms.cgm_session_run_time.cgm_session_run_time, 
			sizeof(u16_t));
		size += sizeof(u16_t);
	}
	
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
			 &gatt_cgms.cgm_session_run_time, size);
}

static ssize_t cgms_cgm_start_time_write(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	u8_t *value = attr->user_data;
	struct cgms_cgm_session_start_time *session_start_time = (struct cgms_cgm_session_start_time *)buf;

	if (session_start_time->time_zone < -48 || session_start_time->time_zone > 56) {
		return BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE);
	}

	if (gatt_cgms.cgm_feature.cgm_feature & CGM_FEAT_E2E_CRC_SUPPORTED) {
		if (offset + len ==  (sizeof(gatt_cgms.cgm_session_start_time) -2 )) {
			return BT_GATT_ERR(CGMS_ERR_MISSING_CRC);
		} else if (offset + len == sizeof(gatt_cgms.cgm_session_start_time)) {
			u16_t crc = crc16_ccitt((u8_t *)&gatt_cgms.cgm_session_start_time, sizeof(gatt_cgms.cgm_session_start_time) - 2);
			if (crc != session_start_time->e2e_crc) {
				return BT_GATT_ERR(CGMS_ERR_INVALID_CRC);
			}
		}
	}
	
	if (offset + len > sizeof(gatt_cgms.cgm_session_start_time)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	return len;
}

#define CGM_SOCP_MAX_OPERAND_LEN 10
struct cgm_socp {
	u8_t opcode;
	u8_t operand[CGM_SOCP_MAX_OPERAND_LEN];
	u8_t operand_len;
	u16_t e2e_crc;
};

struct k_delayed_work cgms_racp_work_item;
struct k_delayed_work cgms_socp_work_item;

void cgms_socp_decode(u8_t data_len, const u8_t *buf, struct cgm_socp *socp_val)
{
    socp_val->opcode = 0xff;
    socp_val->operand_len = 0;
    
    if (data_len > 0) {
        socp_val->opcode = buf[0];
    }

    if (data_len > 1) {
        socp_val->operand_len = data_len - 1;
	 memcpy(socp_val->operand, &buf[1], socp_val->operand_len);
    }
}

u8_t cgms_socp_encode(const struct cgm_socp *socp_val, u8_t *buf)
{
    u8_t len = 0;
    int     i;

    if (buf != NULL) {
        buf[len++] = socp_val->opcode;

        for (i = 0; i < socp_val->operand_len; i++)  {
            buf[len++] = socp_val->operand[i];
        }
		
	if (gatt_cgms.cgm_feature.cgm_feature & CGM_FEAT_E2E_CRC_SUPPORTED) {
		len += u16_encode(socp_val->e2e_crc, &buf[len]);
	}	
    }

    return len;
}

static struct bt_gatt_indicate_params cgms_socp_ind_params;
static u8_t socp_indicating;
static u8_t socp_buf[CGM_SOCP_MAX_OPERAND_LEN+1]  = {0};
static bool start_session = false;
static void socp_indicate_rsp(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, u8_t err)
{
	socp_indicating = 0;
}

static void socp_send(struct cgm_socp *socp_val);

struct cgm_socp socp_response;
static ssize_t cgms_cgm_spec_ops_ctrl_point_write(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	struct  cgm_socp cgms_socp_request ;
	//struct cgm_socp socp_response;

	if (len) {	
		int i = 0;
		
		printk("cgm spec ops ctrl point write: ");
		for (i = 0; i < len; i++) {
			printk("%02x ", ((u8_t *)buf)[i]);
		}
		printk("\n ");
	}
#if 0	
	/* some master does not config the socp ccc */
	if (start_session) {
			if (!gatt_cgms.socp_ccc) {
				printk("socp_ccc not configured\n");
				return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);		
		}
	}
#endif
	cgms_socp_decode(len, (u8_t *)buf, &cgms_socp_request);

	/* here we only support CGM Communication Interval procedures */
	switch (cgms_socp_request.opcode) {
	case SOCP_OPCODE_SET_CGM_COMMUNICATION_INTERVAL:
		socp_response.opcode = SOCP_OPCODE_RESPONSE_CODE;
		socp_response.operand[0] = SOCP_OPCODE_SET_CGM_COMMUNICATION_INTERVAL;
		socp_response.operand[1] = SOCP_RESPONSE_SUCCESS;
		socp_response.operand_len = 2;	

		if (cgms_socp_request.operand_len == 1) {
			communication_interval  = ((u8_t *)buf)[1];
		} else {
			socp_response.operand[0] = SOCP_RESPONSE_INVALID_OPERAND;
			socp_response.operand_len = 1;	
		}	
		
		break;
	case SOCP_OPCODE_GET_CGM_COMMUNICATION_INTERVAL:
		if (cgms_socp_request.operand_len == 0) {
			socp_response.operand[0] = communication_interval;
		} else {
			socp_response.operand[0] = SOCP_RESPONSE_INVALID_OPERAND;
		}
		socp_response.opcode = SOCP_OPCODE_CGM_COMMUNICATION_INTERVAL_RESPONSE;
		socp_response.operand_len = 1;
		break;
	case SOCP_OPCODE_START_THE_SESSION:
		socp_response.opcode = SOCP_OPCODE_START_THE_SESSION;
		if (cgms_socp_request.operand_len == 0) {
			socp_response.operand[0] = SOCP_RESPONSE_SUCCESS;
			start_session = true;
			cgms_set_session_start_time();			
		} else {
			socp_response.operand[0] = SOCP_RESPONSE_INVALID_OPERAND;
		}
		socp_response.operand_len = 1;
		break;		
	default:
		socp_response.opcode = SOCP_OPCODE_RESPONSE_CODE;
		socp_response.operand[0] = SOCP_RESPONSE_OPCODE_UNSUPPORTED;
		socp_response.operand_len = 1;	
		break;
	}

	if (gatt_cgms.cgm_feature.cgm_feature & CGM_FEAT_E2E_CRC_SUPPORTED) {
		socp_response.e2e_crc = crc16_ccitt((u8_t *)&socp_response, sizeof(socp_response.opcode) + socp_response.operand_len);
	}	
	
	/* send CGM Specific Ops Control Point reponse */
	k_delayed_work_submit(&cgms_socp_work_item, 400);

	return len;
}

void cgms_socp_handle(struct k_work *work)
{
	/* send CGM Specific Ops Control Point reponse */
	socp_send(&socp_response);
}

/* Running Speed and Cadence Service Declaration */
static struct bt_gatt_attr cgms_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CGMS),

	BT_GATT_CHARACTERISTIC(BT_UUID_CGMS_CGM_MEASUREMENT, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_CGMS_CGM_MEASUREMENT, BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(cgms_cgm_meas_ccc_cfg, cgms_cgm_meas_ccc_cfg_changed),

	BT_GATT_CHARACTERISTIC(BT_UUID_CGMS_CGM_FEATURE, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_CGMS_CGM_FEATURE, BT_GATT_PERM_READ,
			   cgms_cgm_feature_read, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_CGMS_CGM_STATUS, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_CGMS_CGM_STATUS, BT_GATT_PERM_READ,
			   cgms_cgm_status_read, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_CGMS_CGM_SESSION_START_TIME, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_CGMS_CGM_SESSION_START_TIME, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			   cgms_cgm_start_time_read, cgms_cgm_start_time_write, &gatt_cgms.cgm_session_start_time),

	BT_GATT_CHARACTERISTIC(BT_UUID_CGMS_CGM_SESSION_RUN_TIME, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_CGMS_CGM_SESSION_RUN_TIME, BT_GATT_PERM_READ,
			   cgms_cgm_run_time_read, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_CGMS_RECORD_ACCESS_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE),
	BT_GATT_DESCRIPTOR(BT_UUID_CGMS_RECORD_ACCESS_CONTROL_POINT, BT_GATT_PERM_WRITE, NULL,
			   cgms_racp_write, NULL),
	BT_GATT_CCC(cgms_racp_ccc_cfg, cgms_racp_ccc_cfg_changed),

	BT_GATT_CHARACTERISTIC(BT_UUID_CGMS_CGM_SPECIFIC_OPS_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE),
	BT_GATT_DESCRIPTOR(BT_UUID_CGMS_CGM_SPECIFIC_OPS_CONTROL_POINT, BT_GATT_PERM_WRITE, NULL,
			   cgms_cgm_spec_ops_ctrl_point_write, NULL),
	BT_GATT_CCC(cgms_cgm_spec_ops_ctrl_point_ccc_cfg, cgms_cgm_spec_ops_ctrl_point_ccc_cfg_changed),
};

static struct bt_gatt_service cgms_svc = BT_GATT_SERVICE(cgms_attrs);

/**@brief Function for sending a indication/response from the CGM Specific Ops Control Point.
 *
 * @param[in] socp_val  SOCP value to be sent.
*/
static void socp_send(struct cgm_socp *socp_val)
{
	u8_t len;
	
	/* send indication */
	if (/* gatt_cgms.socp_ccc && */!socp_indicating) {   	/* some master does not config the socp ccc */
		memset(socp_buf, 0, sizeof(socp_buf));

		len = cgms_socp_encode(socp_val, socp_buf);
		cgms_socp_ind_params.attr = &cgms_attrs[16];
		cgms_socp_ind_params.func = socp_indicate_rsp;
		cgms_socp_ind_params.data = socp_buf;
		cgms_socp_ind_params.len = len;

		printk("socp send\n");
		if (!bt_gatt_indicate(NULL, &cgms_socp_ind_params)) {
           		socp_indicating = 1;
		}
	}
}

/**@brief Continuous Glucose Monitoring Service communication state */
enum cgms_state {
    STATE_NO_COMM,                   /* The service is not in a communicating state */
    STATE_RACP_PROC_ACTIVE,          /* Processing requested data */
};

/* record access control point */
struct cgms_racp {
	u8_t opcode;
	u8_t operator;
       u8_t operand_len;     /* length of the operand. */
	u8_t operand[6];       //max len happens at <Filter Type><minimum><maximum>
};

static struct bt_gatt_indicate_params cgms_racp_ind_params;
static u8_t racp_indicating;
static u8_t racp_buf[CGM_MEAS_MAX_SIZE]  = {0};
static struct cgms_racp cgms_racp_request;
static u8_t cgms_racp_response_code;

static enum cgms_state      racp_state;                            /* Current communication state */
static u8_t          racp_proc_operator;                          /* Operator of current request */
static u16_t         racp_proc_time_offset;                           /* Sequence number of current request */
static u8_t          racp_proc_record_idx;                        /* Current record index */
static u8_t          racp_proc_records_reported;                  /* Number of reported records */

void cgms_racp_decode(u8_t data_len, const u8_t *buf, struct cgms_racp *racp_val)
{
    racp_val->opcode      = 0xFF;
    racp_val->operator    = 0xFF;
    racp_val->operand_len = 0;
    
    if (data_len > 0) {
        racp_val->opcode = buf[0];
    }
    if (data_len > 1) {
        racp_val->operator = buf[1];
    }
    if (data_len > 2) {
        racp_val->operand_len = data_len - 2;
	 memcpy(racp_val->operand, &buf[2], racp_val->operand_len);
    }
}

u8_t cgms_racp_encode(const struct cgms_racp *racp_val, u8_t *buf)
{
    u8_t len = 0;
    int     i;

    if (buf != NULL) {
        buf[len++] = racp_val->opcode;
        buf[len++] = racp_val->operator;

        for (i = 0; i < racp_val->operand_len; i++)  {
            buf[len++] = racp_val->operand[i];
        }
    }

    return len;
}

static u8_t cgms_meas_encode(struct cgms_cgm_measurement *cgm_meas, u8_t * buf)
{
	int len = 0;

	buf[len++] = cgm_meas->size;
	buf[len++] = cgm_meas->flag;

	u16_t cgm_glucose_concentration = (cgm_meas->cgm_glucose_concentration.exponent << 12)
		| ((cgm_meas->cgm_glucose_concentration.mantissa & 0x0fff));
	len +=u16_encode(cgm_glucose_concentration, &buf[len]);

	len +=u16_encode(cgm_meas->time_offset, &buf[len]);

	/* here shall be only an octet attached in which at least one bit number is set to "1" */
	if (gatt_cgms.cgm_meas.flag & CGM_SENSOR_STAT_ANNU_STAT_OCTET_PRESENT){
		buf[len++] = gatt_cgms.cgm_meas.sensor_status_annunciation & 0x000000ff;
	}

	if (gatt_cgms.cgm_meas.flag & CGM_SENSOR_STAT_ANNU_CAL_OR_TEMP_OCTET_PRESENT) {
		buf[len++] = (gatt_cgms.cgm_meas.sensor_status_annunciation & 0x0000ff00) >> 8;
	}

	if (gatt_cgms.cgm_meas.flag & CGM_SENSOR_STAT_ANNU_WARNING_OCTET_PRESENT) {
		buf[len++] = (gatt_cgms.cgm_meas.sensor_status_annunciation & 0x00ff0000) >> 16;
	}

	if (cgm_meas->flag & CGM_TREND_INFORMATION_PRESENT) {
		u16_t cgm_trend_information = (cgm_meas->cgm_trend_information.exponent << 12)
			| ((cgm_meas->cgm_trend_information.mantissa & 0x0fff));
		len +=u16_encode(cgm_trend_information, &buf[len]);
	}

	if (cgm_meas->flag & CGM_QUALITY_PRESENT) {
		u16_t cgm_quality = (cgm_meas->cgm_quality.exponent << 12)
			| ((cgm_meas->cgm_quality.mantissa & 0x0fff));
		len +=u16_encode(cgm_quality, &buf[len]);
	}
	
	if (gatt_cgms.cgm_feature.cgm_feature & CGM_FEAT_E2E_CRC_SUPPORTED) {
		cgm_meas->e2e_crc = crc16_ccitt(buf, len);
		len +=u16_encode(cgm_meas->e2e_crc, &buf[len]);
	}
	
	cgm_meas->size = len;
	buf[0] = cgm_meas->size;

	return len;
}


/**@brief Function for setting the CGMS communication state.
 *
 * @param[in] new_state  New communication state.
 */
static void state_set(enum cgms_state new_state)
{
    racp_state = new_state;
}

static void cgms_indicate_rsp(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, u8_t err)
{
	racp_indicating = 0;
	
	/* Make sure state machine returns to the default state */
	state_set(STATE_NO_COMM);
}

/**@brief Function for sending a indication/response from the Record Access Control Point.
 *
 * @param[in] racp_val  RACP value to be sent.
*/
static void racp_send(struct cgms_racp *racp_val)
{
	u8_t len;

	/* send indication */
	if (gatt_cgms.racp_ccc && !racp_indicating) {
		memset(racp_buf, 0, sizeof(racp_buf));

		len = cgms_racp_encode(racp_val, racp_buf);
		cgms_racp_ind_params.attr = &cgms_attrs[13];
		cgms_racp_ind_params.func = cgms_indicate_rsp;
		cgms_racp_ind_params.data = racp_buf;
		cgms_racp_ind_params.len = len;

		printk("racp send\n");
		if (!bt_gatt_indicate(NULL, &cgms_racp_ind_params)) {
           		racp_indicating = 1;
		}
	}
}

/**@brief Function for sending a RACP response containing a Response Code Op Code and a Response Code Value.
 *
 * @param[in] opcode  RACP Op Code.
 * @param[in] value   RACP Response Code Value.
*/
static void racp_response_code_send(u8_t opcode, u8_t value)
{
    struct cgms_racp racp_val;
	
    racp_val.opcode      = RACP_OPCODE_RESPONSE_CODE;
    racp_val.operator    = RACP_OPERATOR_NULL;
    racp_val.operand_len = 2;
    racp_val.operand[0] = opcode;
    racp_val.operand[1] = value;
	
    racp_send(&racp_val);
}

/**@brief Function for sending a cgm measurement notification
 *
 * @param[in] record  Measurement to be sent.
 *
 * @return 0 on success, otherwise an error code.
 */
static int cgm_meas_send(struct cgms_record *record)
{
	u32_t err;
	u8_t buf[CGM_MEAS_MAX_SIZE];
	u16_t len;

	len = cgms_meas_encode(&record->meas, buf);

	/* send notification */
	if ((err = bt_gatt_notify(NULL, &cgms_attrs[2], buf, len)) != 0) {
		printk("cgm measurement notify failed (err %d)\n", err);
	} else {
		racp_proc_records_reported++;
	}

	return err;
}

/**@brief Function for responding to the ALL operation.
 *
 * @return 0 on success, otherwise an error code.
 */
static int racp_report_records_all(void)
{
    u16_t total_records = cgms_db_num_records_get();

    if (racp_proc_record_idx >= total_records)
    {
        state_set(STATE_NO_COMM);
    }
    else
    {
        u32_t      err;
        struct cgms_record rec;

        err = cgms_db_record_get(racp_proc_record_idx, &rec);	
        if (err != 0)
        {
            return err;
        }

        err = cgm_meas_send(&rec);
        if (err != 0)
        {
            return err;
        }
		 /* sleep 300 ms */
		 k_sleep(300);
    }

    return 0;
}

/**@brief Function for responding to the FIRST or the LAST operation.
 *
 * @return  0 on success, otherwise an error code.
*/
static u32_t racp_report_records_first_last(void)
{
    u32_t      err;
    struct cgms_record rec;
    u16_t      total_records;

    total_records = cgms_db_num_records_get();

    if ((racp_proc_records_reported != 0) || (total_records == 0))
    {
        state_set(STATE_NO_COMM);
    }
    else
    {
        if (racp_proc_operator == RACP_OPERATOR_FIRST)
        {
            err = cgms_db_record_get(0, &rec);
            if (err != 0)
            {
                return err;
            }
        }
        else if (racp_proc_operator == RACP_OPERATOR_LAST)
        {
            err = cgms_db_record_get(total_records - 1, &rec);
            if (err != 0)
            {
                return err;
            }
        }

        err = cgm_meas_send(&rec);
        if (err != 0)
        {
            return err;
        }
    }

    return 0;
}

/**@brief Function for responding to the GREATER_OR_EQUAL operation.
 *
 * @return 0 on success, otherwise an error code.
*/
static u32_t racp_report_records_greater_or_equal(void)
{
    u16_t total_records = cgms_db_num_records_get();

    while (racp_proc_record_idx < total_records)
    {
        u32_t      err;
        struct cgms_record rec;

        err = cgms_db_record_get(racp_proc_record_idx, &rec);
        if (err != 0)
        {
            return err;
        }

        if (rec.meas.time_offset >= racp_proc_time_offset)
        {
            err = cgm_meas_send(&rec);
			            if (err != 0)
            {
                return err;
            }
            break;
        }
        racp_proc_record_idx++;
    }
	
    if (racp_proc_record_idx == total_records)
    {
        state_set(STATE_NO_COMM);
    }

    return 0;
}

/**@brief Function for informing that the REPORT RECORDS procedure is completed.
 */
static void racp_report_records_completed(void)
{
    u8_t resp_code_value;

    if (racp_proc_records_reported > 0)
    {
        resp_code_value = RACP_RESPONSE_SUCCESS;
    }
    else
    {
        resp_code_value = RACP_RESPONSE_NO_RECORDS_FOUND;
    }

    racp_response_code_send(RACP_OPCODE_REPORT_RECS, resp_code_value);
}

/**@brief Function for the RACP report records procedure.
 */
static void racp_report_records_procedure(void)
{
	int err;

	while (racp_state == STATE_RACP_PROC_ACTIVE)
	{
		if (cgms_racp_request.opcode != RACP_OPCODE_REPORT_RECS) {
			break;
		}
		
		// Execute requested procedure
		switch (racp_proc_operator)
		{
		case RACP_OPERATOR_ALL:
			err = racp_report_records_all();
			break;

		case RACP_OPERATOR_FIRST:
		case RACP_OPERATOR_LAST:
			err = racp_report_records_first_last();
			break;

		case RACP_OPERATOR_GREATER_OR_EQUAL:
			err = racp_report_records_greater_or_equal();
			break;

		default:
			/* make sure state machine returns to the default state */
			state_set(STATE_NO_COMM);
			return;
		}

		/* error handling */
		if (!err) {
			if (racp_state == STATE_RACP_PROC_ACTIVE) {
				racp_proc_record_idx++;
			} else {
				racp_report_records_completed();
			}
		} else {
			// Make sure state machine returns to the default state
			state_set(STATE_NO_COMM);
		}    
	}
}

/**@brief Function for testing if the received request is to be executed.
 *
 * @param[in]  racp_request   Request to be checked.
 * @param[out] response_code  Response code to be sent in case the request is rejected.
 *                              RACP_RESPONSE_RESERVED is returned if the received message is
 *                              to be rejected without sending a response.
 *
 * @return TRUE if the request is to be executed, FALSE if it is to be rejected.
 *         If it is to be rejected, response_code will contain the response code to be
 *         returned to the central.
 */

/* oprand format:
	within range of Operator: the Operand has the format <Filter Type><minimum><maximum> 
*/
static bool is_request_to_be_executed(const struct cgms_racp *racp_request, u8_t *response_code)
{
	*response_code = RACP_RESPONSE_RESERVED;  // 0
	
	if (racp_request->opcode == RACP_OPCODE_ABORT_OPERATION) {
		if (racp_state == STATE_RACP_PROC_ACTIVE) {
			if (racp_request->operator != RACP_OPERATOR_NULL) {
				*response_code = RACP_RESPONSE_INVALID_OPERATOR;
			} else if (racp_request->operand_len != 0) {
				*response_code = RACP_RESPONSE_INVALID_OPERAND;
			} else {
				*response_code = RACP_RESPONSE_SUCCESS;
			}
		} else {
			*response_code = RACP_RESPONSE_ABORT_FAILED;
		}		
    } else if (racp_state != STATE_NO_COMM) {
        return false;
    } else if ((racp_request->opcode == RACP_OPCODE_REPORT_RECS) ||
		 (racp_request->opcode == RACP_OPCODE_REPORT_NUM_RECS)) {
		/* supported opcodes */
		switch (racp_request->operator) {
		/* operators without a filter */
		case RACP_OPERATOR_ALL:
		case RACP_OPERATOR_FIRST:
		case RACP_OPERATOR_LAST:
		    if (racp_request->operand_len != 0) {
		        *response_code = RACP_RESPONSE_INVALID_OPERAND;
		    }
		    break;

		/* operators with a filter */
		case RACP_OPERATOR_GREATER_OR_EQUAL:
		    if (racp_request->operand[0] == OPERAND_FILTER_TYPE_SEQ_NUM)  {
			 /* <Filter Type><time_offset(2bytes)> */
		        if (racp_request->operand_len != 3)  {
		            *response_code = RACP_RESPONSE_INVALID_OPERAND;
		        }
		    } else if (racp_request->operand[0] == OPERAND_FILTER_TYPE_FACING_TIME) {
		        *response_code = RACP_RESPONSE_OPERAND_UNSUPPORTED;
		    } else if (racp_request->operand[0] >= OPERAND_FILTER_TYPE_RFU_START) {
		        *response_code = RACP_RESPONSE_OPERAND_UNSUPPORTED;
		    } else {
		        *response_code = RACP_RESPONSE_INVALID_OPERAND;
		    }
		    break;

		/* unsupported operators */
		case RACP_OPERATOR_LESS_OR_EQUAL:
		case RACP_OPERATOR_RANGE:
		    *response_code = RACP_RESPONSE_OPERATOR_UNSUPPORTED;
		     break;

		/* invalid operators */
		case RACP_OPERATOR_NULL:
		default:
			/* reserve for future use is not support */
		    if (racp_request->operator >= RACP_OPERATOR_RFU_START) {
		        *response_code = RACP_RESPONSE_OPERATOR_UNSUPPORTED;
		    } else {
		    	/* invalid operators */
		        *response_code = RACP_RESPONSE_INVALID_OPERATOR;
		    }
		    break;
	   }
    }
     /* unsupported opcodes */
    else if (racp_request->opcode == RACP_OPCODE_DELETE_RECS) {
	/* supported opcodes */
	switch (racp_request->operator) 
	{
		/* supported operators */
		case RACP_OPERATOR_ALL:
		case RACP_OPERATOR_FIRST:
		case RACP_OPERATOR_LAST:
		    if (racp_request->operand_len != 0) {
		        *response_code = RACP_RESPONSE_INVALID_OPERAND;
		    }
		    break;

		/* unsupported operators */
		case RACP_OPERATOR_GREATER_OR_EQUAL:
		case RACP_OPERATOR_LESS_OR_EQUAL:
		case RACP_OPERATOR_RANGE:
		    *response_code = RACP_RESPONSE_OPERATOR_UNSUPPORTED;
		     break;

		/* invalid operators */
		case RACP_OPERATOR_NULL:
		default:
			/* reserve for future use is not support */
		    if (racp_request->operator >= RACP_OPERATOR_RFU_START) {
		        *response_code = RACP_RESPONSE_OPERATOR_UNSUPPORTED;
		    } else {
		    	/* invalid operators */
		        *response_code = RACP_RESPONSE_INVALID_OPERATOR;
		    }
		    break;
  	   }
     }  else {
	/* RACP_OPCODE_RESPONSE_CODE and RACP_OPCODE_NUM_RECS_RESPONSE 
	  * are used to reponse
	  */
        *response_code = RACP_RESPONSE_OPCODE_UNSUPPORTED;
    } 

     /* if exception hanppens, the response_code value will change */
    return (*response_code == RACP_RESPONSE_RESERVED);
}

/**@brief Function for processing a REPORT RECORDS request.
 *
 * @param[in] racp_request  Request to be executed.
 */
static void report_records_request_execute(struct cgms_racp *racp_request)
{
	u16_t time_offset = (racp_request->operand[2] << 8) | racp_request->operand[1];

	state_set(STATE_RACP_PROC_ACTIVE);

	racp_proc_record_idx       = 0;
	racp_proc_operator         = racp_request->operator;
	racp_proc_records_reported = 0;
	racp_proc_time_offset          = time_offset;

	racp_report_records_procedure();
}

/**@brief Function for deleting RECORDS request.
 *
 * @param[in] racp_request  Request to be executed.
 */
static void delete_records_request_execute(struct cgms_racp * racp_request)
{
	u16_t total_records;
	u16_t del_num_records;
	u8_t resp_code_value;
	struct cgms_racp racp_val;

	total_records = cgms_db_num_records_get();
	del_num_records   = 0;

	if (racp_request->operator == RACP_OPERATOR_ALL) {
		del_num_records = total_records;
		cgms_db_record_delete_all();
	} else if (racp_request->operator == RACP_OPERATOR_FIRST) {
		if (!cgms_db_record_delete_first()) {
			del_num_records = 1;
		}
	} else if (racp_request->operator == RACP_OPERATOR_LAST) {
		if (!cgms_db_record_delete_last()) {
			del_num_records = 1;
		}
	}

	if (del_num_records > 0) {
		resp_code_value = RACP_RESPONSE_SUCCESS;
	} else {
		resp_code_value = RACP_RESPONSE_NO_RECORDS_FOUND;
	}	

	racp_val.opcode      = RACP_OPCODE_RESPONSE_CODE;
	racp_val.operator    = RACP_OPERATOR_NULL;
	racp_val.operand_len = 2;
	racp_val.operand[0] = RACP_OPCODE_DELETE_RECS;
	racp_val.operand[1] = resp_code_value;

	racp_send(&racp_val);
}

/**@brief Function for processing a REPORT NUM RECORDS request.
 *
 * @param[in] racp_request  Request to be executed.
 */
static void report_num_records_request_execute(struct cgms_racp * racp_request)
{
	u16_t total_records;
	u16_t num_records;
	u32_t      err;
	struct cgms_record rec;
	u16_t time_offset;
	u16_t i;
	struct cgms_racp racp_val;
		
	total_records = cgms_db_num_records_get();
	num_records   = 0;
	
	if (racp_request->operator == RACP_OPERATOR_ALL) {
		num_records = total_records;
	} 

	else if (racp_request->operator == RACP_OPERATOR_GREATER_OR_EQUAL) {
		time_offset = (racp_request->operand[2] << 8) | racp_request->operand[1];

		for (i = 0; i < total_records; i++) {
			err = cgms_db_record_get(i, &rec);
			if (err != 0) {
				return;
			}

			if (rec.meas.time_offset >= time_offset) {
				num_records++;
			}
		}
	}
	else if ((racp_request->operator == RACP_OPERATOR_FIRST) 
		|| (racp_request->operator == RACP_OPERATOR_LAST)) {
		if (total_records > 0) {
			num_records = 1;
		}
	}

	racp_val.opcode      = RACP_OPCODE_NUM_RECS_RESPONSE;
	racp_val.operator    = RACP_OPERATOR_NULL;
	racp_val.operand_len = sizeof(u16_t);
	racp_val.operand[0] = num_records & 0xFF;
	racp_val.operand[1] = num_records >> 8;

	racp_send(&racp_val);
}

void cgms_racp_handle(struct k_work *work)
{
	/* check if request is to be executed */
	if (is_request_to_be_executed(&cgms_racp_request, &cgms_racp_response_code))
	{
		/* execute request */
		if (cgms_racp_request.opcode == RACP_OPCODE_REPORT_RECS) {
			report_records_request_execute(&cgms_racp_request);
		} else if (cgms_racp_request.opcode == RACP_OPCODE_REPORT_NUM_RECS) {
			report_num_records_request_execute(&cgms_racp_request);
		}else if (cgms_racp_request.opcode == RACP_OPCODE_DELETE_RECS) {
			delete_records_request_execute(&cgms_racp_request);
		}
	} else if (cgms_racp_response_code != RACP_RESPONSE_RESERVED) {
		/* abort any running procedure */
		state_set(STATE_NO_COMM);
		/*  respond with error code */
		racp_response_code_send(cgms_racp_request.opcode, cgms_racp_response_code);
	} else {
		printk("procedure already in progress\n");
	}	
}

static ssize_t cgms_racp_write(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset, u8_t flags)
{	
	if (!gatt_cgms.racp_ccc) {
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);		
	}
	
	/* decode request */
	cgms_racp_decode(len, (u8_t *)buf, &cgms_racp_request);

 	if (cgms_racp_request.opcode != RACP_OPCODE_ABORT_OPERATION 
		&& racp_state != STATE_NO_COMM) {
		printk("cgms_err_procedure_in_progress");
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}
	
	if (cgms_racp_request.opcode == RACP_OPCODE_REPORT_RECS){
		printk("cgms_racp_write opcode: report records\n");
	} else if (cgms_racp_request.opcode == RACP_OPCODE_DELETE_RECS){
		printk("cgms_racp_write opcode: delete records\n");
	} else if (cgms_racp_request.opcode == RACP_OPCODE_ABORT_OPERATION){
		printk("cgms_racp_write opcode: abort operation\n");
	} else if (cgms_racp_request.opcode == RACP_OPCODE_REPORT_NUM_RECS){
		printk("cgms_racp_write opcode: report number of records\n");
	} else {
		printk("cgms_racp_write opcode: unsupported\n");
	}
	//printk("racp_request->operator=%d\n", cgms_racp_request.operator);

	/* handle the operations here */	
	k_delayed_work_submit(&cgms_racp_work_item, 400);

	return len;
}

static struct k_timer cgms_timer;
static u16_t session_run_time;

static void cgm_measurement_simulation(void)
{
	struct cgms_record new_record;
	
	/* 1. cgm measurement */
	gatt_cgms.cgm_meas.flag = CGM_TREND_INFORMATION_PRESENT
		| CGM_QUALITY_PRESENT
		| CGM_SENSOR_STAT_ANNU_WARNING_OCTET_PRESENT
		| CGM_SENSOR_STAT_ANNU_CAL_OR_TEMP_OCTET_PRESENT
		| CGM_SENSOR_STAT_ANNU_STAT_OCTET_PRESENT;

	gatt_cgms.cgm_meas.cgm_glucose_concentration.exponent = 0;
	gatt_cgms.cgm_meas.cgm_glucose_concentration.mantissa = 18.02 * 5;  /* 5 mol/L */

	gatt_cgms.cgm_meas.time_offset = session_run_time;
	gatt_cgms.cgm_meas.sensor_status_annunciation = 0 
		//	| CGM_MEAS_SESSION_STOPPED
		//	| CGM_MEAS_DEVICE_BATTERY_LOW
		//	| CGM_MEAS_TYPE_INCORRECT
		//	| CGM_MEAS_SENSOR_MALFUNCTION
		//	| CGM_MEAS_DEVICE_SPECIFIC_ALERT
		//	| CGM_MEAS_GENERAL_DEVICE_FAULT
			| CGM_MEAS_TIME_SYNCHRONIZATION_REQUIRED
		//	| CGM_MEAS_CALIBRATION_NOT_ALLOWED
		//	| CGM_MEAS_CALIBRATION_RECOMMENDED
		//	| CGM_MEAS_CALIBRATION_REQUIRED
		//	| CGM_MEAS_TEMPERATURE_TOO_HIGH
		//	| CGM_MEAS_TEMPERATURE_TOO_LOW
		//	| CGM_MEAS_RESULT_LOWER_THAN_LOW_LEVEL
		//	| CGM_MEAS_RESULT_HIGHER_THAN_HIGH_LEVEL
		//	| CGM_MEAS_RESULT_LOWER_THAN_HYPO_LEVEL
		//	| CGM_MEAS_RESULT_HIGHER_THAN_HYPER_LEVEL
		//	| CGM_MEAS_RATE_OF_DECREASE_EXCEEDED
		//	| CGM_MEAS_RATE_OF_INCREASE_EXCEEDED
		//	| CGM_MEAS_RESULT_LOWER_THAN_CAN_PROCESS
		//	| CGM_MEAS_RESULT_HIGHER_THAN_CAN_PROCESS
		;

	gatt_cgms.cgm_meas.cgm_trend_information.exponent = 0;
	gatt_cgms.cgm_meas.cgm_trend_information.mantissa = 3; 

	gatt_cgms.cgm_meas.cgm_quality.exponent = 0;
	gatt_cgms.cgm_meas.cgm_quality.mantissa = 4; 

	/* 2.update the CGM Status value
       * Sensor Status Annunciation field shall form part of every CGM Measurement Record to which it applies.
       * here shall be only an octet attached in which at least one bit number is set to "1" */
	u8_t buf[CGM_STATUS_MAX_SIZE] = {0};
	int len = 0;
	u8_t status = 0, cal_temp = 0, warning = 0;
	
	gatt_cgms.cgm_status.time_offset = gatt_cgms.cgm_meas.time_offset;
	len += u16_encode(gatt_cgms.cgm_status.time_offset, &buf[len]);

	/* The Status Annunciation Field always consists of three octets regardless the value */
	buf[len++] = status = gatt_cgms.cgm_meas.sensor_status_annunciation & 0x000000ff;
	buf[len++] = cal_temp = (gatt_cgms.cgm_meas.sensor_status_annunciation & 0x0000ff00) >> 8;
	
	buf[len++] = warning = (gatt_cgms.cgm_meas.sensor_status_annunciation & 0x00ff0000) >> 16;
	gatt_cgms.cgm_status.sensor_status = (warning << 16) | (cal_temp << 8) | status;
	if (gatt_cgms.cgm_feature.cgm_feature & CGM_FEAT_E2E_CRC_SUPPORTED) {
		gatt_cgms.cgm_status.e2e_crc = crc16_ccitt(buf, len);
	}

	new_record.meas = gatt_cgms.cgm_meas;;
	/* add to database */
	if (cgms_db_record_add(&new_record)) {
		printk("add to database failed, no space available!\n");
	}
}

void cgm_measurement_notify(void)
{
	if (gatt_cgms.cgm_meas_ccc) {
		int err;
		int len = 0;
		u8_t buf[CGM_MEAS_MAX_SIZE]  = {0};

		len = cgms_meas_encode(& gatt_cgms.cgm_meas, buf);
		if (0 != (err = bt_gatt_notify(NULL, &cgms_attrs[2], buf, len))) {
			printk("cgm measurement notify failed (err %d)\n", err);
		}	
	}
}

void cgms_timer_handler(struct k_timer *dummy)
{
	session_run_time++;
	
	/* cgms measurement */
	if ((session_run_time % 5 == 0) && (cgms_db_num_records_get() < CGMS_DB_MAX_RECORDS)) {
		printk("generate continuous glucose measurement\n");
		cgm_measurement_simulation();
	}
	
	if (communication_interval == 0xff) {
		/* There are two ways that a client can receive notifications of the CGM Measurement Characteristic value:
		* Periodic Notifications: CGM Communication Interval, 0x0 - disable, 0xff most fast
		* Requested Notifications:
		*/
		cgm_measurement_notify();  // most fast
	} else if (communication_interval && (session_run_time % communication_interval == 0)) {
		cgm_measurement_notify();
	}

	/* update the runtime */
	gatt_cgms.cgm_session_run_time.cgm_session_run_time = session_run_time / 60;
}

static void cgm_measurement_delete(void) 
{
	cgms_db_record_delete_all();
}

static void cgm_measurement_show(void) 
{
	printk("database records number: %d\n", cgms_db_num_records_get());
}

static void cgms_crc_enable(bool enable)
{
	if (enable) {
		gatt_cgms.cgm_feature.cgm_feature  |= CGM_FEAT_E2E_CRC_SUPPORTED;
	} else {
		gatt_cgms.cgm_feature.cgm_feature  &= ~CGM_FEAT_E2E_CRC_SUPPORTED;
	}
}

static struct k_timer cgms_timer;

void cgmss_timer_init(void)
{
	k_timer_init(&cgms_timer, cgms_timer_handler, NULL); 
} 

void cgmss_timer_start(void)
{
	k_timer_start(&cgms_timer, K_SECONDS(1), K_SECONDS(1)); 
} 

/*
Upon initial connection, if the device supports an automatic start of the CGM session (e.g., at power on),
or after the Start Session procedure, the Client shall write its current time to this characteristic and the
Server shall calculate and store the Session Start Time using the time of the client and its own current
relative time value.
*/
/* The Session Start time is calculated and set by the server at the particular time of the
* initial connection and should not be changed afterwards. 
*/
void cgms_set_session_start_time(void)
{
	gatt_cgms.cgm_session_start_time.session_start_time.year = 2018;
	gatt_cgms.cgm_session_start_time.session_start_time.month = 4;
	gatt_cgms.cgm_session_start_time.session_start_time.day = 19;
	gatt_cgms.cgm_session_start_time.session_start_time.hours = 14;
	gatt_cgms.cgm_session_start_time.session_start_time.minutes = 42;
	gatt_cgms.cgm_session_start_time.session_start_time.seconds = 22;

	gatt_cgms.cgm_session_start_time.time_zone = 8 * 4;  // 15min * 4 = 1h
	gatt_cgms.cgm_session_start_time.dst_offset = DST_OFFSET_STANDARD_TIME;
	
	if (gatt_cgms.cgm_feature.cgm_feature & CGM_FEAT_E2E_CRC_SUPPORTED) {
		gatt_cgms.cgm_session_start_time.e2e_crc = crc16_ccitt((u8_t *)&gatt_cgms.cgm_session_start_time, 
			sizeof(struct cgms_cgm_session_start_time) - 2);
	}
	
	/* reset the session run time */
	session_run_time = 0;

	/* start timer */
	cgmss_timer_start();
	
	/* deletes all data from database */
	cgm_measurement_delete();
}

void cgms_init(void)
{
	gatt_cgms.cgm_feature.cgm_feature  = CGM_FEAT_PATIENT_HIGH_LOW_ALERTS_SUPPORTED
		//| CGM_FEAT_CALIBRATION_SUPPORTED
		//| CGM_FEAT_HYPO_ALERTS_SUPPORTED	
		//| CGM_FEAT_HYPER_ALERTS_SUPPORTED
		//| CGM_FEAT_RATE_OF_INCREASE_DECREASE_ALERTS_SUPPORTED
		//| CGM_FEAT_DEVICE_SPECIFIC_ALERT_SUPPORTED
		| CGM_FEAT_SENSOR_MALFUNCTION_DETECTION_SUPPORTED
		| CGM_FEAT_SENSOR_TEMPERATURE_HIGH_LOW_DETECTION_SUPPORTED
		| CGM_FEAT_SENSOR_RESULT_HIGH_LOW_DETECTION_SUPPORTED
		//| CGM_FEAT_LOW_BATTERY_DETECTION_SUPPORTED
		| CGM_FEAT_SENSOR_TYPE_ERROR_DETECTION_SUPPORTED
		| CGM_FEAT_GENERAL_DEVICE_FAULT_SUPPORTED
		//| CGM_FEAT_E2E_CRC_SUPPORTED
		//| CGM_FEAT_MULTIPLE_BOND_SUPPORTED
		| CGM_FEAT_MULTIPLE_SESSIONS_SUPPORTED
		| CGM_FEAT_CGM_TREND_INFORMATION_SUPPORTED
		| CGM_FEAT_CGM_QUALITY_SUPPORTED;

	gatt_cgms.cgm_feature.cgm_type_sample_location =
		(CGM_TYPE_CAPILLARY_WHOLE_BLOOD << 4) | CGM_SAMPLE_LOCAL_FINGER;

	if (gatt_cgms.cgm_feature.cgm_feature & CGM_FEAT_E2E_CRC_SUPPORTED) {
		gatt_cgms.cgm_feature.e2e_crc = crc16_ccitt((u8_t *)&gatt_cgms.cgm_feature, 
			sizeof(gatt_cgms.cgm_feature.cgm_feature) + sizeof(gatt_cgms.cgm_feature.cgm_type_sample_location));
	}

	/* initialize database */ 
	cgms_db_init();

	/* set initial communicating state */
	state_set(STATE_NO_COMM);
	
	bt_gatt_service_register(&cgms_svc);

	k_delayed_work_init(&cgms_racp_work_item, cgms_racp_handle);
	k_delayed_work_init(&cgms_socp_work_item, cgms_socp_handle);

	cgmss_timer_init();
}

