/** @file
 *  @brief GLS Service sample
 */

/*
 * Copyright (c) 2018 Actions Semiconductor, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _GLS_H
#define _GLS_H
#include <zephyr/types.h>
#include <bluetooth/conn.h>
#include <date_time.h>
#include <iso_ieee_11073.h>

#ifdef __cplusplus
extern "C" {
#endif

/* glucose measurement flags */
#define GLS_MEAS_FLAG_TIME_OFFSET                  0x01    /* time offset present */
#define GLS_MEAS_FLAG_CONC_TYPE_LOC                0x02    /* glucose concentration, type, and sample location present */
#define GLS_MEAS_FLAG_UNITS_KG_L                   0x00    /* glucose concentration units kg/l */
#define GLS_MEAS_FLAG_UNITS_MOL_L                  0x04    /* glucose concentration units mol/l */
#define GLS_MEAS_FLAG_SENSOR_STATUS                0x08    /* sensor status annunciation present */
#define GLS_MEAS_FLAG_CONTEXT_INFO                 0x10    /* context information follows */

/* glucose measurement type */
#define GLS_MEAS_TYPE_CAP_BLOOD                    1     /* capillary whole blood */
#define GLS_MEAS_TYPE_CAP_PLASMA                   2     /* capillary plasma */
#define GLS_MEAS_TYPE_VEN_BLOOD                    3     /* venous whole blood */
#define GLS_MEAS_TYPE_VEN_PLASMA                   4     /* venous plasma */
#define GLS_MEAS_TYPE_ART_BLOOD                    5     /* arterial whole blood */
#define GLS_MEAS_TYPE_ART_PLASMA                   6     /* arterial plasma */
#define GLS_MEAS_TYPE_UNDET_BLOOD                  7     /* undetermined whole blood */
#define GLS_MEAS_TYPE_UNDET_PLASMA                 8     /* undetermined plasma */
#define GLS_MEAS_TYPE_INTERS_FLUID                 9     /* interstitial fluid (isf) */
#define GLS_MEAS_TYPE_CONTROL                      10    /* control solution */

/* glucose measurement location */
#define GLS_MEAS_LOC_FINGER                        1     /* finger */
#define GLS_MEAS_LOC_AST                           2     /* alternate site test (ast) */
#define GLS_MEAS_LOC_EAR                           3     /* earlobe */
#define GLS_MEAS_LOC_CONTROL                       4     /* control solution */
#define GLS_MEAS_LOC_NOT_AVAIL                     15    /* sample location value not available */

/* glucose sensor status annunciation */
#define GLS_MEAS_STATUS_BATT_LOW                   0x0001    /* device battery low at time of measurement */
#define GLS_MEAS_STATUS_SENSOR_FAULT               0x0002    /* sensor malfunction or faulting at time of measurement */
#define GLS_MEAS_STATUS_SAMPLE_SIZE                0x0004    /* sample size for blood or control solution insufficient at time of measurement */
#define GLS_MEAS_STATUS_STRIP_INSERT               0x0008    /* strip insertion error */
#define GLS_MEAS_STATUS_STRIP_TYPE                 0x0010    /* strip type incorrect for device */
#define GLS_MEAS_STATUS_RESULT_HIGH                0x0020    /* sensor result higher than the device can process */
#define GLS_MEAS_STATUS_RESULT_LOW                 0x0040    /* sensor result lower than the device can process */
#define GLS_MEAS_STATUS_TEMP_HIGH                  0x0080    /* sensor temperature too high for valid test/result at time of measurement */
#define GLS_MEAS_STATUS_TEMP_LOW                   0x0100    /* sensor temperature too low for valid test/result at time of measurement */
#define GLS_MEAS_STATUS_STRIP_PULL                 0x0200    /* sensor read interrupted because strip was pulled too soon at time of measurement */
#define GLS_MEAS_STATUS_GENERAL_FAULT              0x0400    /* general device fault has occurred in the sensor */
#define GLS_MEAS_STATUS_TIME_FAULT                 0x0800    /* time fault has occurred in the sensor and time may be inaccurate */

/* glucose measurement context flags */
#define GLS_CONTEXT_FLAG_CARB                      0x01    /* carbohydrate id and carbohydrate present */
#define GLS_CONTEXT_FLAG_MEAL                      0x02    /* meal present */
#define GLS_CONTEXT_FLAG_TESTER                    0x04    /* tester-health present */
#define GLS_CONTEXT_FLAG_EXERCISE                  0x08    /* exercise duration and exercise intensity present */
#define GLS_CONTEXT_FLAG_MED                       0x10    /* medication id and medication present */
#define GLS_CONTEXT_FLAG_MED_KG                    0x00    /* medication value units, kilograms */
#define GLS_CONTEXT_FLAG_MED_L                     0x20    /* medication value units, liters */
#define GLS_CONTEXT_FLAG_HBA1C                     0x40    /* hba1c present */
#define GLS_CONTEXT_FLAG_EXT                       0x80    /* extended flags present */

/* glucose measurement context carbohydrate id */              
#define GLS_CONTEXT_CARB_ID_BREAKFAST              1    /* breakfast */
#define GLS_CONTEXT_CARB_ID_LUNCH                  2    /* lunch */
#define GLS_CONTEXT_CARB_ID_DINNER                 3    /* dinner */
#define GLS_CONTEXT_CARB_ID_SNACK                  4    /* snack */
#define GLS_CONTEXT_CARB_ID_DRINK                  5    /* drink */
#define GLS_CONTEXT_CARB_ID_SUPPER                 6    /* supper */
#define GLS_CONTEXT_CARB_ID_BRUNCH                 7    /* brunch */

/* glucose measurement context meal */
#define GLS_CONTEXT_MEAL_PREPRANDIAL               1    /* preprandial (before meal) */
#define GLS_CONTEXT_MEAL_POSTPRANDIAL              2    /* postprandial (after meal) */
#define GLS_CONTEXT_MEAL_FASTING                   3    /* fasting */
#define GLS_CONTEXT_MEAL_CASUAL                    4    /* casual (snacks, drinks, etc.) */
#define GLS_CONTEXT_MEAL_BEDTIME                   5    /* bedtime */

/* glucose measurement context tester */
#define GLS_CONTEXT_TESTER_SELF                    1     /* self */
#define GLS_CONTEXT_TESTER_PRO                     2     /* health care professional */
#define GLS_CONTEXT_TESTER_LAB                     3     /* lab test */
#define GLS_CONTEXT_TESTER_NOT_AVAIL               15    /* tester value not available */

/* glucose measurement context health */
#define GLS_CONTEXT_HEALTH_MINOR                   1     /* minor health issues */
#define GLS_CONTEXT_HEALTH_MAJOR                   2     /* major health issues */
#define GLS_CONTEXT_HEALTH_MENSES                  3     /* during menses */
#define GLS_CONTEXT_HEALTH_STRESS                  4     /* under stress */
#define GLS_CONTEXT_HEALTH_NONE                    5     /* no health issues */
#define GLS_CONTEXT_HEALTH_NOT_AVAIL               15    /* health value not available */

/* glucose measurement context medication id */
#define GLS_CONTEXT_MED_ID_RAPID                   1    /* rapid acting insulin */
#define GLS_CONTEXT_MED_ID_SHORT                   2    /* short acting insulin */
#define GLS_CONTEXT_MED_ID_INTERMED                3    /* intermediate acting insulin */
#define GLS_CONTEXT_MED_ID_LONG                    4    /* long acting insulin */
#define GLS_CONTEXT_MED_ID_PREMIX                  5    /* pre-mixed insulin */

/* glucose feature */
#define GLS_FEATURE_LOW_BATT                       0x0001    /* low battery detection during measurement supported */
#define GLS_FEATURE_MALFUNC                        0x0002    /* sensor malfunction detection supported */
#define GLS_FEATURE_SAMPLE_SIZE                    0x0004    /* sensor sample size supported */
#define GLS_FEATURE_INSERT_ERR                     0x0008    /* sensor strip insertion error detection supported */
#define GLS_FEATURE_TYPE_ERR                       0x0010    /* sensor strip type error detection supported */
#define GLS_FEATURE_RES_HIGH_LOW                   0x0020    /* sensor result high-low detection supported */
#define GLS_FEATURE_TEMP_HIGH_LOW                  0x0040    /* sensor temperature high-low detection supported */
#define GLS_FEATURE_READ_INT                       0x0080    /* sensor read interrupt detection supported */
#define GLS_FEATURE_GENERAL_FAULT                  0x0100    /* general device fault supported */
#define GLS_FEATURE_TIME_FAULT                     0x0200    /* time fault supported */
#define GLS_FEATURE_MULTI_BOND                     0x0400    /* multiple bond supported */

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

/* glucose measurement structure */
struct gls_glucose_meas {
    u8_t   flags;                              /* flags */
    u16_t  sequence_number;                    /* sequence number,  cann't roll over */
    struct date_time base_time;                /* time stamp */
    s16_t  time_offset;                        /* time offset */
    ieee_sfloat glucose_concentration;  /* glucose concentration, unavailable value is NaN */
    u8_t   type;                               /* type, use only four bits */
    u8_t   sample_location;                    /* sample location, use only four bits */
    u16_t  sensor_status_annunciation;         /* sensor status annunciation */
};

/* glucose measurement context structure */
struct gls_meas_context {
	u8_t   flags;                       /* flags */
  	u16_t  sequence_number;        /* sequence number */
	u8_t   extended_flags;              /* extended flags */
	u8_t   carbohydrate_id;             /* carbohydrate id */
	ieee_sfloat carbohydrate;    /* carbohydrate */
	u8_t   meal;                        /* meal */
	u8_t   tester_and_health;           /* tester and health */
	u16_t  exercise_duration;           /* exercise duration */
	u8_t   exercise_intensity;          /* exercise intensity */
	u8_t   medication_id;               /* medication id */
	ieee_sfloat medication;      /* medication */
	ieee_sfloat hba1c;           /* hba1c */
};

/* record access control point */
struct gls_racp {
	u8_t opcode;
	u8_t operator;
       u8_t operand_len;     /* length of the operand. */
	u8_t operand[6];       //max len happens at <Filter Type><minimum><maximum>
};

/* glucose service struct */
struct bt_gatt_gls{
	struct gls_glucose_meas glucose_meas;
	u8_t   glucose_meas_ccc;
	struct gls_meas_context context;
	u8_t   context_ccc;
	u16_t glucose_feature;
	struct gls_racp racp;	
	u8_t racp_ccc;	
};

/* glucose measurement record */
struct gls_record {
	struct gls_glucose_meas  meas;
	struct gls_meas_context  context;
};

#pragma pack()

int gls_init(void);
void gls_state_reset(void) ;

#ifdef __cplusplus
}
#endif

#endif    /* _GLS_H */
