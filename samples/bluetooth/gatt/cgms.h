/** @file
 *  @brief CGMS Service sample
 */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _CGMS_H
#define _CGMS_H
#include <zephyr/types.h>
#include <bluetooth/conn.h>
#include <iso_ieee_11073.h>
#include <date_time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* record access control point op code */
#define RACP_OPCODE_RESERVED                           0       /* reserved for future use. */
#define RACP_OPCODE_REPORT_RECS                1    /* report stored records */
#define RACP_OPCODE_DELETE_RECS                2    /* delete stored records */
#define RACP_OPCODE_ABORT_OPERATION               3	  /* abort operation */
#define RACP_OPCODE_REPORT_NUM_RECS         4    /* report number of stored records */
#define RACP_OPCODE_NUM_RECS_RESPONSE       5    /* number of stored records response */
#define RACP_OPCODE_RESPONSE_CODE                 6    /* response code */

/* record access control point operator */
#define RACP_OPERATOR_NULL             0    /* null */
#define RACP_OPERATOR_ALL      1    /* all records */
#define RACP_OPERATOR_LESS_OR_EQUAL    2    /* less than or equal to */
#define RACP_OPERATOR_GREATER_OR_EQUAL 3    /* greater than or equal to */
#define RACP_OPERATOR_RANGE     4    /* within range of (inclusive) */
#define RACP_OPERATOR_FIRST     5    /* first record */
#define RACP_OPERATOR_LAST      6    /* last record */
#define RACP_OPERATOR_RFU_START   7    /* start of reserved for future use area */

/* record access control point operand op code/operand value correspondence */
#define RACP_OPERAND_OPCODE_FILTER_PARAM1         1    /* filter parameters */
#define RACP_OPERAND_OPCODE_FILTER_PARAM2         2    /* filter parameters */
#define RACP_OPERAND_OPCODE_NOT_INCLUDED          3    /* not included */
#define RACP_OPERAND_OPCODE_FILTER_PARAM4         4    /* filter parameters */
#define RACP_OPERAND_OPCODE_RECORDS_NUMBER        5    /* number of records */
#define RACP_OPERAND_OPCODE_REQ_OPCODE_RESP_CODE 6    /* request op code, response code value */

/* record access control point operand response code values */
#define RACP_RESPONSE_RESERVED              0       /* reserved for future use. */
#define RACP_RESPONSE_SUCCESS                 1    /* success	normal */
#define RACP_RESPONSE_OPCODE_UNSUPPORTED   2    /* op code not supported */
#define RACP_RESPONSE_INVALID_OPERATOR        3    /* invalid operator */
#define RACP_RESPONSE_OPERATOR_UNSUPPORTED  4    /* operator not supported */
#define RACP_RESPONSE_INVALID_OPERAND         5    /* invalid operand */
#define RACP_RESPONSE_NO_RECORDS_FOUND        6    /* no records found */
#define RACP_RESPONSE_ABORT_FAILED      7    /* abort unsuccessful */
#define RACP_RESPONSE_PROCEDURE_NOT_DONE 8    /* procedure not completed */
#define RACP_RESPONSE_OPERAND_UNSUPPORTED   9    /* operand not supported */

#define OPERAND_FILTER_TYPE_SEQ_NUM     0x01                                     /**< Filter data using Sequence Number criteria. */
#define OPERAND_FILTER_TYPE_FACING_TIME 0x02                                     /**< Filter data using User Facing Time criteria. */
#define OPERAND_FILTER_TYPE_RFU_START   0x07                                     /**< Start of filter types reserved For Future Use range */
#define OPERAND_FILTER_TYPE_RFU_END     0xFF                                     /**< End of filter types reserved For Future Use range */

#pragma pack(1)
struct  cgms_cgm_session_run_time {
	u16_t cgm_session_run_time;
	u16_t e2e_crc;
};

struct  cgms_cgm_session_start_time {
	struct date_time session_start_time;
	s8_t time_zone;
	u8_t dst_offset;
	u16_t e2e_crc;  /* crc16 */
};

struct  cgms_cgm_feature {
	u32_t cgm_feature;      //only use 24 bit
	u8_t cgm_type_sample_location; // type: 4 bit, sample_location:4 bit
	u16_t e2e_crc;  /* crc16 */
};

struct  cgms_cgm_measurement {
	u8_t size;   //  the size of the CGM Measurement record
	u8_t flag;
	ieee_sfloat  cgm_glucose_concentration;   // unit in mg/dl 
	u16_t time_offset;  // unit  in minutes 
	u32_t sensor_status_annunciation;  //only use three octets
	ieee_sfloat cgm_trend_information;
	ieee_sfloat cgm_quality;
	u16_t e2e_crc; /* crc16 */
};

struct  cgms_cgm_status {
	u16_t time_offset;  // unit  in minutes 
	u32_t sensor_status; //only use 24 bit
	u16_t e2e_crc; /* crc16 */
};

struct  cgm_socp_calibration_value  {
	u16_t glucose_concentration; // the type is ieee_sfloat
	u16_t calibration_time;
	u8_t type_sample_location; // type: 4 bit, sample_location:4 bit
	u16_t next_calibration_time;
	u16_t calibration_data_record_number;
	u8_t calibration_status;
};

struct bt_gatt_cgms {
	struct  cgms_cgm_measurement cgm_meas;
	u8_t cgm_meas_ccc;
	struct cgms_cgm_feature cgm_feature;  // lifetime static
	struct cgms_cgm_status cgm_status;     // the same to cgm measurement 
	struct cgms_cgm_session_start_time cgm_session_start_time;
	struct cgms_cgm_session_run_time cgm_session_run_time;
	u8_t racp_ccc;
	u8_t socp_ccc;
};

/* Continuous Glucose Monitoring Service record */
struct cgms_record {
	struct  cgms_cgm_measurement meas;
};
#pragma pack()

void cgms_init(void);
void cgms_set_session_start_time(void);

#ifdef __cplusplus
}
#endif

#endif    /* _CGMS_H */
