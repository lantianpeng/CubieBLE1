/** @file
 *  @brief GLS Service sample
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
#include "gls_db.h"
#include "gls.h"

/* gls error code */
#define GLS_ERR_CCC_IMPROPER_CONF  0x81
#define GLS_ERR_PROCEDURE_IN_PROGRESS  0x80

/**@brief Glucose Service communication state */
enum gls_state {
    STATE_NO_COMM,                   /* The service is not in a communicating state */
    STATE_RACP_PROC_ACTIVE,          /* Processing requested data */
};

#define GLS_BUF_SIZE 32

static struct bt_gatt_gls gatt_gls;
static struct bt_gatt_ccc_cfg gls_glucose_meas_ccc_cfg[BT_GATT_CCC_MAX];
static struct bt_gatt_ccc_cfg gls_glucose_meas_context_ccc_cfg[BT_GATT_CCC_MAX];
static struct bt_gatt_ccc_cfg gls_racp_ccc_cfg[BT_GATT_CCC_MAX];

static struct bt_gatt_indicate_params gls_racp_ind_params;
static u8_t racp_indicating;
static u8_t racp_buf[GLS_BUF_SIZE]  = {0};
static struct gls_racp gls_racp_request;
static u8_t gls_racp_response_code;

struct k_delayed_work gls_delay_work_item;
static struct k_timer gls_timer;
static s16_t systick;

static u16_t         record_next_seq_num;                         /* Sequence number of the next database record */
static enum gls_state      racp_state;                            /* Current communication state */
static u8_t          racp_proc_operator;                          /* Operator of current request */
static u16_t         racp_proc_seq_num;                           /* Sequence number of current request */
static u8_t          racp_proc_record_idx;                        /* Current record index */
static u8_t          racp_proc_records_reported;                  /* Number of reported records */

static ssize_t gls_racp_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, u16_t len, u16_t offset,
				u8_t flags);

void gls_racp_decode(u8_t data_len, u8_t *buf, struct gls_racp *racp_val)
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

u8_t gls_racp_encode(const struct gls_racp *racp_val, u8_t *buf)
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

static void gls_glucose_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{	
	gatt_gls.glucose_meas_ccc = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static void gls_glucose_meas_context_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{	
	gatt_gls.context_ccc = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static void gls_racp_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{	
	gatt_gls.racp_ccc = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

static ssize_t gls_glucose_feature_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &gatt_gls.glucose_feature,
			sizeof(gatt_gls.glucose_feature));
}

/* Glucose Service Declaration */
static struct bt_gatt_attr gls_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GLS),

	BT_GATT_CHARACTERISTIC(BT_UUID_GLS_GLUCOSE_MEASUREMENT, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_GLS_GLUCOSE_MEASUREMENT, BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(gls_glucose_meas_ccc_cfg, gls_glucose_meas_ccc_cfg_changed),

	BT_GATT_CHARACTERISTIC(BT_UUID_GLS_GLUCOSE_MEASUREMENT_CONTEXT, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_GLS_GLUCOSE_MEASUREMENT_CONTEXT, BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(gls_glucose_meas_context_ccc_cfg, gls_glucose_meas_context_ccc_cfg_changed),

	BT_GATT_CHARACTERISTIC(BT_UUID_GLS_GLUCOSE_FEATURE, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_GLS_GLUCOSE_FEATURE, BT_GATT_PERM_READ,
		gls_glucose_feature_read, NULL, NULL),	

	BT_GATT_CHARACTERISTIC(BT_UUID_GLS_RECORD_ACCESS_CONTROL_POINT, BT_GATT_CHRC_WRITE| BT_GATT_CHRC_INDICATE),
	BT_GATT_DESCRIPTOR(BT_UUID_GLS_RECORD_ACCESS_CONTROL_POINT, BT_GATT_PERM_WRITE_ENCRYPT, NULL, gls_racp_write, NULL),
	BT_GATT_CCC(gls_racp_ccc_cfg, gls_racp_ccc_cfg_changed),
};

static struct bt_gatt_service gls_svc = BT_GATT_SERVICE(gls_attrs);

/**@brief Function for setting the GLS communication state.
 *
 * @param[in] new_state  New communication state.
 */
static void state_set(enum gls_state new_state)
{
    racp_state = new_state;
}

/**@brief Function for setting the next sequence number by reading the last record in the data base.
 *
 * @return 0 on successful initialization of service, otherwise an error code.
 */
static u32_t next_sequence_number_set(void)
{
    u16_t      num_records;
    struct gls_record rec;

    num_records = gls_db_num_records_get();
    if (num_records > 0) {
        /* get last record */ 
        u32_t err_code = gls_db_record_get(num_records - 1, &rec);
        if (err_code != 0) {
            return err_code;
        }
        record_next_seq_num = rec.meas.sequence_number + 1;
    }
    else {
        record_next_seq_num = 0;
    }

    return 0;
}

/**@brief Function for encoding a Glucose measurement.
 *
 * @param[in]  p_meas            Measurement to be encoded.
 * @param[out] p_encoded_buffer  Pointer to buffer where the encoded measurement is to be stored.
 *
 * @return Size of encoded measurement.
 */
static u8_t gls_meas_encode(const struct gls_glucose_meas *glucose_meas, u8_t * buf)
{
	u8_t len = 0;

	buf[len++] = glucose_meas->flags;
	len +=u16_encode(glucose_meas->sequence_number, &buf[len]);
	len +=date_time_encode(&glucose_meas->base_time, &buf[len]);

	if (glucose_meas->flags | GLS_MEAS_FLAG_TIME_OFFSET) {
		len +=u16_encode(glucose_meas->time_offset, &buf[len]);
	}

	if (glucose_meas->flags | GLS_MEAS_FLAG_CONC_TYPE_LOC) {
		u16_t glucose_concentration = (glucose_meas->glucose_concentration.exponent << 12)
			| ((glucose_meas->glucose_concentration.mantissa & 0x0fff));
		u8_t type_sample_location = (glucose_meas->type << 4)
			| glucose_meas->sample_location;
		len +=u16_encode(glucose_concentration, &buf[len]);
		buf[len++] = type_sample_location;
	}

	if (glucose_meas->flags | GLS_MEAS_FLAG_SENSOR_STATUS) {
		len +=u16_encode(glucose_meas->sensor_status_annunciation, &buf[len]);
	}

    return len;
}

static u8_t gls_meas_context_encode(const struct gls_meas_context *context, u8_t * buf)
{
	u8_t len = 0;

	buf[len++] = context->flags;
	len +=u16_encode(context->sequence_number, &buf[len]);

	if (context->flags | GLS_CONTEXT_FLAG_EXT) {
		buf[len++] = context->extended_flags;
	}	

	if (context->flags | GLS_CONTEXT_FLAG_CARB) {
		u16_t carbohydrate = (context->carbohydrate.exponent << 12)
			| ((context->carbohydrate.mantissa & 0x0fff));
		buf[len++] = context->carbohydrate_id;
		len +=u16_encode(carbohydrate, &buf[len]);
	}	

	if (context->flags | GLS_CONTEXT_FLAG_MEAL) {
		buf[len++] = context->meal;
	}	

	if (context->flags | GLS_CONTEXT_FLAG_TESTER) {
		buf[len++] = context->tester_and_health;
	}	

	if (context->flags | GLS_CONTEXT_FLAG_EXERCISE) {
		len +=u16_encode(context->exercise_duration, &buf[len]);
		buf[len++] = context->exercise_intensity;
	}	

	if (context->flags | GLS_CONTEXT_FLAG_MED) {
		u16_t medication = (context->medication.exponent << 12)
			| ((context->medication.mantissa & 0x0fff));
		buf[len++] = context->medication_id;
		len +=u16_encode(medication, &buf[len]);
	}	

	if (context->flags | GLS_CONTEXT_FLAG_HBA1C) {
		u16_t hba1c = (context->hba1c.exponent << 12)
			| ((context->hba1c.mantissa & 0x0fff));
		len +=u16_encode(hba1c, &buf[len]);
	}
	
	return len;
}

static void gls_indicate_rsp(struct bt_conn *conn,
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
static void racp_send(struct gls_racp *racp_val)
{
	u8_t len;

	/* send indication */
	if (gatt_gls.racp_ccc && !racp_indicating) {
		memset(racp_buf, 0, sizeof(racp_buf));

		len = gls_racp_encode(racp_val, racp_buf);
		gls_racp_ind_params.attr = &gls_attrs[10];
		gls_racp_ind_params.func = gls_indicate_rsp;
		gls_racp_ind_params.data = racp_buf;
		gls_racp_ind_params.len = len;

		printk("racp send\n");
		if (!bt_gatt_indicate(NULL, &gls_racp_ind_params)) {
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
    struct gls_racp racp_val;
	
    racp_val.opcode      = RACP_OPCODE_RESPONSE_CODE;
    racp_val.operator    = RACP_OPERATOR_NULL;
    racp_val.operand_len = 2;
    racp_val.operand[0] = opcode;
    racp_val.operand[1] = value;
	
    racp_send(&racp_val);
}

/**@brief Function for sending a glucose measurement/context notification
 *
 * @param[in] p_gls  Service instance.
 * @param[in] p_rec  Measurement to be sent.
 *
 * @return 0 on success, otherwise an error code.
 */
static int glucose_meas_send(struct gls_record *record)
{
	u32_t err;
	u8_t buf[GLS_BUF_SIZE];
	u16_t len;

	len = gls_meas_encode(&record->meas, buf);

	/* send notification */
	if ((err = bt_gatt_notify(NULL, &gls_attrs[2], buf, len)) != 0) {
		printk("glucose measurement notify failed (err %d)\n", err);
	} else {
		racp_proc_records_reported++;
	}

	len = gls_meas_context_encode(&record->context, buf);
	if (record->meas.flags & GLS_MEAS_FLAG_CONTEXT_INFO) {
		if ((err = bt_gatt_notify(NULL, &gls_attrs[5], buf, len)) != 0) {
			printk("glucose measurement context notify failed (err %d)\n", err);
		} 
	}

	return err;
}

/**@brief Function for responding to the ALL operation.
 *
 * @return 0 on success, otherwise an error code.
 */
static int racp_report_records_all(void)
{
    u16_t total_records = gls_db_num_records_get();

    if (racp_proc_record_idx >= total_records)
    {
        state_set(STATE_NO_COMM);
    }
    else
    {
        u32_t      err;
        struct gls_record rec;

        err = gls_db_record_get(racp_proc_record_idx, &rec);	
        if (err != 0)
        {
            return err;
        }

        err = glucose_meas_send(&rec);
        if (err != 0)
        {
            return err;
        }
	 /* sleep 200 ms */
	 k_sleep(200);
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
    struct gls_record rec;
    u16_t      total_records;

    total_records = gls_db_num_records_get();

    if ((racp_proc_records_reported != 0) || (total_records == 0))
    {
        state_set(STATE_NO_COMM);
    }
    else
    {
        if (racp_proc_operator == RACP_OPERATOR_FIRST)
        {
            err = gls_db_record_get(0, &rec);
            if (err != 0)
            {
                return err;
            }
        }
        else if (racp_proc_operator == RACP_OPERATOR_LAST)
        {
            err = gls_db_record_get(total_records - 1, &rec);
            if (err != 0)
            {
                return err;
            }
        }

        err = glucose_meas_send(&rec);
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
    u16_t total_records = gls_db_num_records_get();

    while (racp_proc_record_idx < total_records)
    {
        u32_t      err;
        struct gls_record rec;

        err = gls_db_record_get(racp_proc_record_idx, &rec);
        if (err != 0)
        {
            return err;
        }

        if (rec.meas.sequence_number >= racp_proc_seq_num)
        {
            err = glucose_meas_send(&rec);
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
 *
 * @param[in] p_gls  Service instance.
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
		if (gls_racp_request.opcode != RACP_OPCODE_REPORT_RECS) {
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
static bool is_request_to_be_executed(const struct gls_racp *racp_request, u8_t *response_code)
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
			 /* <Filter Type><seq_num(2bytes)> */
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
static void report_records_request_execute(struct gls_racp *racp_request)
{
	u16_t seq_num = (racp_request->operand[2] << 8) | racp_request->operand[1];

	state_set(STATE_RACP_PROC_ACTIVE);

	racp_proc_record_idx       = 0;
	racp_proc_operator         = racp_request->operator;
	racp_proc_records_reported = 0;
	racp_proc_seq_num          = seq_num;

	racp_report_records_procedure();
}

/**@brief Function for deleting RECORDS request.
 *
 * @param[in] racp_request  Request to be executed.
 */
static void delete_records_request_execute(struct gls_racp * racp_request)
{
	u16_t total_records;
	u16_t del_num_records;
	u8_t resp_code_value;
	struct gls_racp racp_val;

	total_records = gls_db_num_records_get();
	del_num_records   = 0;

	if (racp_request->operator == RACP_OPERATOR_ALL) {
		del_num_records = total_records;
		gls_db_record_delete_all();
	} else if (racp_request->operator == RACP_OPERATOR_FIRST) {
		if (!gls_db_record_delete_first()) {
			del_num_records = 1;
		}
	} else if (racp_request->operator == RACP_OPERATOR_LAST) {
		if (!gls_db_record_delete_last()) {
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
static void report_num_records_request_execute(struct gls_racp * racp_request)
{
	u16_t total_records;
	u16_t num_records;
	u32_t      err;
	struct gls_record rec;
	u16_t seq_num;
	u16_t i;
	struct gls_racp racp_val;
	
	total_records = gls_db_num_records_get();
	num_records   = 0;
	
	if (racp_request->operator == RACP_OPERATOR_ALL) {
		num_records = total_records;
	} 

	else if (racp_request->operator == RACP_OPERATOR_GREATER_OR_EQUAL) {
		seq_num = (racp_request->operand[2] << 8) | racp_request->operand[1];

		for (i = 0; i < total_records; i++) {
			err = gls_db_record_get(i, &rec);
			if (err != 0) {
				return;
			}

			if (rec.meas.sequence_number >= seq_num) {
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

void gls_racp_handle(struct k_work *work)
{
	printk("gls_racp_handle\n");
	/* check if request is to be executed */
	if (is_request_to_be_executed(&gls_racp_request, &gls_racp_response_code))
	{
		/* execute request */
		if (gls_racp_request.opcode == RACP_OPCODE_REPORT_RECS) {
			report_records_request_execute(&gls_racp_request);
		} else if (gls_racp_request.opcode == RACP_OPCODE_REPORT_NUM_RECS) {
			report_num_records_request_execute(&gls_racp_request);
		}else if (gls_racp_request.opcode == RACP_OPCODE_DELETE_RECS) {
			delete_records_request_execute(&gls_racp_request);
		}
	} else if (gls_racp_response_code != RACP_RESPONSE_RESERVED) {
		/* abort any running procedure */
		state_set(STATE_NO_COMM);
		/*  respond with error code */
		racp_response_code_send(gls_racp_request.opcode, gls_racp_response_code);
	} else {
		printk("procedure already in progress\n");
	}	
}

static ssize_t gls_racp_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, u16_t len, u16_t offset,
				u8_t flags)
{
	printk("gls_racp_write\n");
	
	if (!gatt_gls.racp_ccc) {
		return BT_GATT_ERR(GLS_ERR_CCC_IMPROPER_CONF);		
	}

	if (offset + len > sizeof(gatt_gls.racp)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	/* decode request */
	gls_racp_decode(len, (u8_t *)buf, &gls_racp_request);

 	if (gls_racp_request.opcode != RACP_OPCODE_ABORT_OPERATION 
		&& racp_state != STATE_NO_COMM) {
		return BT_GATT_ERR(GLS_ERR_PROCEDURE_IN_PROGRESS);
	}
	
	if (gls_racp_request.opcode == RACP_OPCODE_REPORT_RECS){
		printk("opcode: report records\n");
	} else if (gls_racp_request.opcode == RACP_OPCODE_DELETE_RECS){
		printk("opcode: delete records\n");
	} else if (gls_racp_request.opcode == RACP_OPCODE_ABORT_OPERATION){
		printk("opcode: abort operation\n");
	} else if (gls_racp_request.opcode == RACP_OPCODE_REPORT_NUM_RECS){
		printk("opcode: report number of records\n");
	}
	//printk("racp_request->operator=%d\n", gls_racp_request.operator);

	k_delayed_work_submit(&gls_delay_work_item, 400);
	
	return len;
}

void set_base_time(struct date_time *date_time)
{
	date_time->year = 2018;
	date_time->month = 4;
	date_time->day = 11;
	date_time->hours = 16;
	date_time->minutes = 6;
	date_time->seconds = 18;
}

enum gls_unit {
	UNIT_KG_L,
	UNIT_MOL_L
};

int glucose_measurement_simulation(u8_t unit, bool with_context)
{
	u8_t unit_val;
	struct gls_record new_record;

	memset(&new_record, 0, sizeof(new_record));
	memset(&gatt_gls.glucose_meas, 0, sizeof(gatt_gls.glucose_meas));	
	
	if (unit == UNIT_KG_L) {
		unit_val = GLS_MEAS_FLAG_UNITS_KG_L;
		gatt_gls.glucose_meas.glucose_concentration.exponent = -5;
		gatt_gls.glucose_meas.glucose_concentration.mantissa = 5; 
	} else {
		gatt_gls.glucose_meas.glucose_concentration.exponent = -3;
		gatt_gls.glucose_meas.glucose_concentration.mantissa = 5;  /* 5 mmol/L */
		unit_val = GLS_MEAS_FLAG_UNITS_MOL_L;
	}
		
	/* glucose measurement  */
	gatt_gls.glucose_meas.flags = GLS_MEAS_FLAG_TIME_OFFSET
		| GLS_MEAS_FLAG_CONC_TYPE_LOC | unit_val
		| GLS_MEAS_FLAG_SENSOR_STATUS;
	gatt_gls.glucose_meas.sequence_number = ++record_next_seq_num;
	set_base_time(&gatt_gls.glucose_meas.base_time);
	gatt_gls.glucose_meas.time_offset = systick;
	gatt_gls.glucose_meas.type = GLS_MEAS_TYPE_VEN_BLOOD;
	gatt_gls.glucose_meas.sample_location = GLS_MEAS_LOC_FINGER;
	gatt_gls.glucose_meas.sensor_status_annunciation = 0;
	
	new_record.meas = gatt_gls.glucose_meas;
	
	if (with_context) {
		/* glucose measurement context present */
		gatt_gls.glucose_meas.flags |= GLS_MEAS_FLAG_CONTEXT_INFO;
		
		/* glucose measurement context */
		memset(&gatt_gls.context, 0, sizeof(gatt_gls.context));
		gatt_gls.context.flags = GLS_CONTEXT_FLAG_CARB
			| GLS_CONTEXT_FLAG_MEAL
			| GLS_CONTEXT_FLAG_TESTER
			| GLS_CONTEXT_FLAG_EXERCISE
			| GLS_CONTEXT_FLAG_MED
			| GLS_CONTEXT_FLAG_HBA1C
			| GLS_CONTEXT_FLAG_EXT;
		gatt_gls.context.sequence_number = gatt_gls.glucose_meas.sequence_number;
		gatt_gls.context.extended_flags = 0;
		gatt_gls.context.carbohydrate_id = GLS_CONTEXT_CARB_ID_BREAKFAST;
		gatt_gls.context.carbohydrate.exponent = -3;
		gatt_gls.context.carbohydrate.mantissa = 90;
		gatt_gls.context.meal = GLS_CONTEXT_MEAL_PREPRANDIAL;
		gatt_gls.context.tester_and_health = (GLS_CONTEXT_HEALTH_NONE << 4)  | GLS_CONTEXT_TESTER_SELF;
		gatt_gls.context.exercise_duration = 1800;
		gatt_gls.context.exercise_intensity = 70;
		gatt_gls.context.medication_id = GLS_CONTEXT_MED_ID_RAPID;
		if (unit_val == GLS_MEAS_FLAG_UNITS_KG_L) {  // use the same unit_val as meas
			gatt_gls.context.medication.exponent = -3;
			gatt_gls.context.medication.mantissa = 3;
			gatt_gls.context.flags |= GLS_CONTEXT_FLAG_MED_KG;
		} else {
			gatt_gls.context.medication.exponent = -3;
			gatt_gls.context.medication.mantissa = 3;
			gatt_gls.context.flags |= GLS_CONTEXT_FLAG_MED_L; 
		}
		gatt_gls.context.hba1c.exponent = 0;
		gatt_gls.context.hba1c.mantissa = 5;
		new_record.meas = gatt_gls.glucose_meas;
		new_record.context = gatt_gls.context;		
	}

	/* add to database */
	if (gls_db_record_add(&new_record)) {
		printk("add to database failed, no space available!\n");
	}

	return 0;
}

void glucose_measurement_delete(void) 
{
	gls_db_record_delete_all();
}

void glucose_measurement_show(void) 
{
	printk("database records number: %d\n", gls_db_num_records_get());
}

void gls_state_reset(void) {
	state_set(STATE_NO_COMM);
}

void gls_timer_handler(struct k_timer *dummy)
{
	systick++;
	if ((systick % 5 == 0) && (gls_db_num_records_get() < GLS_DB_MAX_RECORDS)) {
		printk("generate glucose measurement\n");
		glucose_measurement_simulation(UNIT_MOL_L, false);
	}
}

void gls_timer_init(void) 
{	
	k_timer_init(&gls_timer, gls_timer_handler, NULL);
	k_timer_start(&gls_timer, K_SECONDS(1), K_SECONDS(1));
} 

int gls_init(void)
{
	u32_t   err;
	
	memset(&gatt_gls, 0, sizeof(gatt_gls));
	gatt_gls.glucose_feature =  GLS_FEATURE_LOW_BATT
            | GLS_FEATURE_MALFUNC
            | GLS_FEATURE_RES_HIGH_LOW
            | GLS_FEATURE_GENERAL_FAULT
            | GLS_FEATURE_TIME_FAULT
            | GLS_FEATURE_MULTI_BOND;   // lifetime static

	/* initialize data base */ 
	gls_db_init();
	err = next_sequence_number_set();
	if (err != 0) {
		return err;
	}

	bt_gatt_service_register(&gls_svc);

	gls_timer_init();
	
	k_delayed_work_init(&gls_delay_work_item, gls_racp_handle);
	
	state_set(STATE_NO_COMM);	
	
	return 0;
}


