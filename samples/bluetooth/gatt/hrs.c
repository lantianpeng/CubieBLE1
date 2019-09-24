/** @file
 *  @brief HRS Service sample
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

/* Heart Rate Measurement initial value */
#define HRM_INITIAL_VALUE                       100 
#define HRM_MAX_VALUE                           280

/* Heart Rate Measurement flag bits */
#define HRM_FLAG_MASK_HR_VALUE_16BIT           (0x01 << 0) 
#define HRM_FLAG_MASK_SENSOR_CONTACT_DETECTED  (0x01 << 1)
#define HRM_FLAG_MASK_SENSOR_CONTACT_SUPPORTED (0x01 << 2)
#define HRM_FLAG_MASK_EXPENDED_ENERGY_REPRESENT (0x01 << 3)
#define HRM_FLAG_MASK_RR_INTERVAL_REPRESENT     (0x01 << 4)

/* Body Sensor Location values */
#define BT_HRS_BODY_SENSOR_LOCATION_OTHER      0
#define BT_HRS_BODY_SENSOR_LOCATION_CHEST      1
#define BT_HRS_BODY_SENSOR_LOCATION_WRIST      2
#define BT_HRS_BODY_SENSOR_LOCATION_FINGER     3
#define BT_HRS_BODY_SENSOR_LOCATION_HAND       4
#define BT_HRS_BODY_SENSOR_LOCATION_EAR_LOBE   5
#define BT_HRS_BODY_SENSOR_LOCATION_FOOT       6

/* Heart Rate Control Point */
#define BT_HRS_CONTROL_POINT_RESERVED      0
#define BT_HRS_CONTROL_POINT_RESET_ENERGY_EXPENDED      1
#define HRS_ERR_CONTROL_POINT_NOT_SUPPORT      0x80

/* Size of RR Interval buffer inside service */
#define BT_HRS_MAX_BUFFERED_RR_INTERVALS       20

static u8_t simulate_hrm;
static u8_t simulate_cnt = 0;
static u16_t heartrate = 254;

struct bt_gatt_hrm {
	u8_t flags;
	u16_t hrm_value;
	u16_t energy_expended;
	u16_t rr_interval[BT_HRS_MAX_BUFFERED_RR_INTERVALS];
	u16_t rr_interval_count;
} __packed;

#define BT_GATT_HRM_MAX_SIZE 45
static struct bt_gatt_ccc_cfg hrmc_ccc_cfg[BT_GATT_CCC_MAX] = {};

struct bt_gatt_hrs {
	struct bt_gatt_hrm hrm;
	u8_t body_sensor_location;
	u8_t heart_rate_control_point;
	bool is_heart_rate_value_format_uint16;   /* true if the Heart Rate Value format is sent in a UINT8 format */
	bool is_sensor_contact_detected;                /* true if sensor contact has been detected */
	bool is_sensor_contact_supported;              /* true if sensor contact detection is supported */
	bool is_energy_expended_present;          /* true if Expended Energy measurement is supported */
	bool is_rr_interval_present;				/* true if RR-Interval values are present */
} __packed;

static struct bt_gatt_hrs gatt_hrs;

struct sensorsim_cfg {
    u32_t min;
    u32_t max;
    u32_t incr;
    bool start_at_max;
};

struct sensorsim_state {
    u32_t current_val;
    bool is_increasing;
};

void sensorsim_init(struct sensorsim_state * sensor_state,
		const struct sensorsim_cfg * sensor_cfg)
{
    if (sensor_cfg->start_at_max) {
        sensor_state->current_val   = sensor_cfg->max;
        sensor_state->is_increasing = false;
    } else {
        sensor_state->current_val   = sensor_cfg->min;
        sensor_state->is_increasing = true;
    }
}

void sensorsim_increment(struct sensorsim_state * sensor_state,
		const struct sensorsim_cfg * sensor_cfg)
{
    if (sensor_cfg->max - sensor_state->current_val > sensor_cfg->incr) {
        sensor_state->current_val += sensor_cfg->incr;
    } else {
        sensor_state->current_val   = sensor_cfg->max;
        sensor_state->is_increasing = false;
    }
}

void sensorsim_decrement(struct sensorsim_state * sensor_state,
	const struct sensorsim_cfg * sensor_cfg)
{
    if (sensor_state->current_val - sensor_cfg->min > sensor_cfg->incr) {
        sensor_state->current_val -= sensor_cfg->incr;
    } else {
        sensor_state->current_val   = sensor_cfg->min;
        sensor_state->is_increasing = true;
    }
}

u32_t sensorsim_measure(struct sensorsim_state * sim_state,
		const struct sensorsim_cfg * sensor_cfg)
{
    if (sim_state->is_increasing) {
        sensorsim_increment(sim_state, sensor_cfg);
    } else {
        sensorsim_decrement(sim_state, sensor_cfg);
    }
    return sim_state->current_val;
}

static void hrmc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	simulate_hrm = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_blsc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &gatt_hrs.body_sensor_location,
				 sizeof(gatt_hrs.body_sensor_location));
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, u16_t len, u16_t offset,
				u8_t flags)
{
	u8_t *value = attr->user_data;
	u8_t *hrcp = (u8_t *)buf;
	
	if (*hrcp != BT_HRS_CONTROL_POINT_RESET_ENERGY_EXPENDED) {
		return BT_GATT_ERR(HRS_ERR_CONTROL_POINT_NOT_SUPPORT);
	} 
	
	if (offset + len > sizeof(gatt_hrs.heart_rate_control_point)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	
	gatt_hrs.hrm.energy_expended = 0;

	return len;
}


static struct sensorsim_cfg         m_energy_expended_sim_cfg;
static struct sensorsim_state       m_energy_expended_sim_state;
static struct sensorsim_cfg         m_rr_interval_sim_cfg;
static struct sensorsim_state       m_rr_interval_sim_state;

#define MIN_ENERGY_EXPENDED         0   
#define MAX_ENERGY_EXPENDED         65535
#define ENERGY_EXPENDED_INCREMENT   10        

#define MIN_RR_INTERVAL             100  
#define MAX_RR_INTERVAL             500  
#define RR_INTERVAL_INCREMENT       1          

static struct k_timer bt_hrs_rr_interval_timer;
static struct k_timer bt_hrs_energy_expended_timer;
void bt_hrs_rr_interval_timer_handler(struct k_timer *dummy)
{
	if (gatt_hrs.is_rr_interval_present) {
		u16_t rr_interval;
		rr_interval = (u16_t)sensorsim_measure(&m_rr_interval_sim_state,  &m_rr_interval_sim_cfg);
		if (gatt_hrs.hrm.rr_interval_count == BT_HRS_MAX_BUFFERED_RR_INTERVALS) {	
			/* The rr_interval buffer is full, delete the oldest value */
			memmove(&gatt_hrs.hrm.rr_interval[0],  &gatt_hrs.hrm.rr_interval[1],
			    (BT_HRS_MAX_BUFFERED_RR_INTERVALS - 1) * sizeof(u16_t));
			gatt_hrs.hrm.rr_interval_count--;
		}

		/* Add new value */
		gatt_hrs.hrm.rr_interval[gatt_hrs.hrm.rr_interval_count++] = rr_interval;
	}
}

void bt_hrs_energy_expended_timer_handler(struct k_timer *dummy)
{
	if (gatt_hrs.is_energy_expended_present) {
		u16_t energy_expended;
		energy_expended = (u16_t)sensorsim_measure(&m_energy_expended_sim_state,  &m_energy_expended_sim_cfg);
		gatt_hrs.hrm.energy_expended += energy_expended;
	}
}

void timer_init(void) {
	k_timer_init(&bt_hrs_rr_interval_timer, bt_hrs_rr_interval_timer_handler, NULL);
	k_timer_start(&bt_hrs_rr_interval_timer, K_SECONDS(1), K_SECONDS(1));

	k_timer_init(&bt_hrs_energy_expended_timer, bt_hrs_energy_expended_timer_handler, NULL);
	k_timer_start(&bt_hrs_energy_expended_timer, K_SECONDS(2), K_SECONDS(2));
}

static void sensor_simulator_init(void)
{
    m_energy_expended_sim_cfg.min  = MIN_ENERGY_EXPENDED;
    m_energy_expended_sim_cfg.max = MAX_ENERGY_EXPENDED;
    m_energy_expended_sim_cfg.incr = ENERGY_EXPENDED_INCREMENT;
    m_energy_expended_sim_cfg.start_at_max = false;
    sensorsim_init(&m_energy_expended_sim_state, &m_energy_expended_sim_cfg);

    m_rr_interval_sim_cfg.min = MIN_RR_INTERVAL;
    m_rr_interval_sim_cfg.max = MAX_RR_INTERVAL;
    m_rr_interval_sim_cfg.incr = RR_INTERVAL_INCREMENT;
    m_rr_interval_sim_cfg.start_at_max = false;

    sensorsim_init(&m_rr_interval_sim_state, &m_rr_interval_sim_cfg);
}

static u8_t u16_encode(u16_t value, u8_t *encoded_data)
{
    encoded_data[0] = (u8_t) ((value & 0x00FF) >> 0);
    encoded_data[1] = (u8_t) ((value & 0xFF00) >> 8);
    return sizeof(u16_t);
}

#define OPCODE_LENGTH 1                                                    /* Length of opcode inside Heart Rate Measurement packet */
#define HANDLE_LENGTH 2                                                    /* Length of handle inside Heart Rate Measurement packet */
#define BLE_L2CAP_MTU_DEF 23
#define MAX_HRM_LEN   (BLE_L2CAP_MTU_DEF - OPCODE_LENGTH - HANDLE_LENGTH)  /* Maximum size of a transmitted Heart Rate Measurement */

static u8_t hrm_encode(struct bt_gatt_hrm *hrm, u8_t *encoded_buffer, bool energy_expended_included)
{
	u8_t len  = 1; 
	int i;

	/* Encode heart rate measurement */
	if (hrm->hrm_value > 0xff) {
	    hrm->flags |= HRM_FLAG_MASK_HR_VALUE_16BIT;
	    len += u16_encode(hrm->hrm_value, &encoded_buffer[len]);
	} else {
	    hrm->flags &= ~HRM_FLAG_MASK_HR_VALUE_16BIT;
	    encoded_buffer[len++] = hrm->hrm_value;
	}

	if (energy_expended_included) {
		if (gatt_hrs.is_energy_expended_present) {
			len += u16_encode(gatt_hrs.hrm.energy_expended, &encoded_buffer[len]);
		}
	} else {
		len += sizeof(gatt_hrs.hrm.energy_expended);
	}

	/* Encode rr_interval values */
	if (hrm->rr_interval_count > 0) {
	    hrm->flags |= HRM_FLAG_MASK_RR_INTERVAL_REPRESENT;
	}
	
	for (i = 0; i < hrm->rr_interval_count; i++) {
		if (len + sizeof(u16_t) > MAX_HRM_LEN)  {
			/* Not all stored rr_interval values can fit into the encoded hrm,
			 * move the remaining values to the start of the buffer.
			 */
			memmove(&hrm->rr_interval[0], &hrm->rr_interval[i],
				(hrm->rr_interval_count - i) * sizeof(u16_t));
			break;
		}

		len += u16_encode(hrm->rr_interval[i], &encoded_buffer[len]);
	}
	
	hrm->rr_interval_count -= i;

	/* Add flags */
	encoded_buffer[0] =  hrm->flags;

	return len;
}

/* Heart Rate Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HRS),

	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_MEASUREMENT, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_HRS_MEASUREMENT, BT_GATT_PERM_READ, NULL, NULL, NULL),
	BT_GATT_CCC(hrmc_ccc_cfg, hrmc_ccc_cfg_changed),

	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_BODY_SENSOR, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_HRS_BODY_SENSOR, BT_GATT_PERM_READ,
			   read_blsc, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_HRS_CONTROL_POINT, BT_GATT_CHRC_WRITE),
	BT_GATT_DESCRIPTOR(BT_UUID_HRS_CONTROL_POINT, BT_GATT_PERM_WRITE,
			   NULL, write_ctrl_point, &gatt_hrs.heart_rate_control_point),
};

static struct bt_gatt_service hrs_svc = BT_GATT_SERVICE(attrs);
void hrs_init(u8_t blsc)
{
	memset(&gatt_hrs, 0 , sizeof(gatt_hrs));
	
	gatt_hrs.is_heart_rate_value_format_uint16 = false;
	gatt_hrs.is_sensor_contact_detected = true;
	gatt_hrs.is_sensor_contact_supported = true;
	gatt_hrs.is_energy_expended_present = true;
	gatt_hrs.is_rr_interval_present = true;

	if (gatt_hrs.is_heart_rate_value_format_uint16) {
		gatt_hrs.hrm.flags |= HRM_FLAG_MASK_HR_VALUE_16BIT;
	}
	
	if (gatt_hrs.is_sensor_contact_detected) {
		gatt_hrs.hrm.flags |= HRM_FLAG_MASK_SENSOR_CONTACT_DETECTED;
	}
	
	if (gatt_hrs.is_sensor_contact_supported) {
		gatt_hrs.hrm.flags |= HRM_FLAG_MASK_SENSOR_CONTACT_SUPPORTED;
	}

	if (gatt_hrs.is_energy_expended_present) {
		gatt_hrs.hrm.flags |= HRM_FLAG_MASK_EXPENDED_ENERGY_REPRESENT;
	}
	
	if (gatt_hrs.is_rr_interval_present) {
		gatt_hrs.hrm.flags |= HRM_FLAG_MASK_RR_INTERVAL_REPRESENT;
	}
	
	gatt_hrs.hrm.hrm_value = HRM_INITIAL_VALUE;
	if (blsc >= BT_HRS_BODY_SENSOR_LOCATION_OTHER && blsc <= BT_HRS_BODY_SENSOR_LOCATION_FOOT)
		gatt_hrs.body_sensor_location = blsc;
	else {
		gatt_hrs.body_sensor_location = BT_HRS_BODY_SENSOR_LOCATION_FINGER;
	}
	gatt_hrs.heart_rate_control_point = BT_HRS_CONTROL_POINT_RESERVED;	

	bt_gatt_service_register(&hrs_svc);

	sensor_simulator_init();
	
	timer_init();
}


void hrs_notify(void)
{
	u8_t hrm_buffer[BT_GATT_HRM_MAX_SIZE] = {0};
	int len = 0;
		
	/* Heartrate measurements simulation */
	if (!simulate_hrm) {
		return;
	}

	if (++heartrate == HRM_MAX_VALUE) {
		heartrate = HRM_INITIAL_VALUE;
	}
	
	gatt_hrs.hrm.hrm_value = heartrate;

	/* encode the hrm */
	len = hrm_encode(&gatt_hrs.hrm, hrm_buffer, gatt_hrs.is_energy_expended_present);
 
	bt_gatt_notify(NULL, &attrs[2], hrm_buffer, len);
}

