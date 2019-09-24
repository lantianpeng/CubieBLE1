/** @file
 *  @brief HTS Service sample
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
#include <date_time.h>

/**@brief FLOAT format (IEEE-11073 32-bit FLOAT, defined as a 32-bit value  
 *         with a 24-bit mantissa and an 8-bit exponent
 *  The number represented is mantissa*(10**exponent) */
typedef struct ieee_float_type {
	s8_t  exponent;       /* base 10 exponent */
	s32_t mantissa;       /* mantissa, should be using only 24 bits */
} ieee_float;

/* HTS temperature  measurement flag bits */
#define HTS_FLAG_TEMPERATURE_UNITS	 0
#define HTS_FLAG_TIME_STAMP	               1
#define HTS_FLAG_TEMPERATURE_TYPE	 2

/* temperature type measurement locations */
#define HTS_TEMP_TYPE_ARMPIT      1
#define HTS_TEMP_TYPE_BODY        2
#define HTS_TEMP_TYPE_EAR         3
#define HTS_TEMP_TYPE_FINGER      4
#define HTS_TEMP_TYPE_GI_TRACT    5
#define HTS_TEMP_TYPE_MOUTH       6
#define HTS_TEMP_TYPE_RECTUM      7
#define HTS_TEMP_TYPE_TOE         8
#define HTS_TEMP_TYPE_EAR_DRUM    9

/* measurement interval range  */
#define HTS_MEASUREMENT_INTERVAL_MIN     1         // 1s
#define HTS_MEASUREMENT_INTERVAL_MAX     0xa8c0    // 12 hours
#define HTS_MEASUREMENT_INTERVAL_DEF      2        // 2s

/* error codes */
#define HTS_ERR_OUT_OF_RANGE     0x80

struct hts_temp_measurement {
	bool   temp_in_fahr_units;                /* true if temperature is in fahrenheit units, celcius otherwise */
	bool   time_stamp_present;                /* true if time stamp is present */
	bool   temp_type_present;                 /* true if temperature type is present */
	union {
		ieee_float celcius;                   /* temperature measurement value (celcius) */
		ieee_float fahrenheit;                /* temperature measurement value (fahrenheit) */
	} temp;
	struct date_time time_stamp;              /* time stamp */
	u8_t   temp_type;                         /* temperature type */
}__packed;

#define HTS_TEMP_MEASUREMENT_SIZE 13

struct hts_valid_range {
	u16_t   lower_inclusive_val;
	u16_t   upper_inclusive_val;
} __packed;

struct bt_gatt_hts {
	struct hts_temp_measurement temp_measurement;
	u8_t temp_measurement_ccc;
	u8_t temp_type;
	struct hts_temp_measurement intermediate_temp;
	u8_t intermediate_temp_ccc;
	u16_t measurement_interval;
	u8_t measurement_interval_ccc;
	struct hts_valid_range valid_range;
 };

static struct bt_gatt_hts gatt_hts;
static struct date_time current_date;
static struct bt_gatt_ccc_cfg hts_temp_measurement_ccc_cfg[BT_GATT_CCC_MAX];
static struct bt_gatt_ccc_cfg hts_intermediate_temp_ccc_cfg[BT_GATT_CCC_MAX];
/* temperature measurement indication params */
static struct bt_gatt_indicate_params hts_temp_measurement_ind_params;
static u8_t htm_indicating;

static void hts_temp_measurement_ccc_cfg_changed(const struct bt_gatt_attr *attr,  u16_t value)
{
	gatt_hts.temp_measurement_ccc = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

static void hts_intermediate_temp_ccc_cfg_changed(const struct bt_gatt_attr *attr,  u16_t value)
{
	gatt_hts.intermediate_temp_ccc = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t hts_read_temp_type(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &gatt_hts.temp_type,
			sizeof(gatt_hts.temp_type));
}

/* Health Thermometer Service Declaration */
static struct bt_gatt_attr hts_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HTS),
		
	BT_GATT_CHARACTERISTIC(BT_UUID_HTS_TEMPERATURE_MEASUREMENT, BT_GATT_CHRC_INDICATE),
	BT_GATT_DESCRIPTOR(BT_UUID_HTS_TEMPERATURE_MEASUREMENT, BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(hts_temp_measurement_ccc_cfg, hts_temp_measurement_ccc_cfg_changed),

	BT_GATT_CHARACTERISTIC(BT_UUID_HTS_TEMPERATURE_TYPE, BT_GATT_CHRC_READ),
	BT_GATT_DESCRIPTOR(BT_UUID_HTS_TEMPERATURE_TYPE, BT_GATT_PERM_READ, hts_read_temp_type, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_HTS_INTERMEDIATE_TEMPERATURE, BT_GATT_CHRC_NOTIFY),
	BT_GATT_DESCRIPTOR(BT_UUID_HTS_INTERMEDIATE_TEMPERATURE, BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(hts_intermediate_temp_ccc_cfg, hts_intermediate_temp_ccc_cfg_changed),	
};

static struct bt_gatt_service hts_svc = BT_GATT_SERVICE(hts_attrs);
static struct k_timer hts_timer;

static void hts_indicate_rsp(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, u8_t err)
{
	htm_indicating = 0;
	printk("hts_indicate_rsp: err 0x%02x\n", err);
}

static u8_t temp_measurement_buf[HTS_TEMP_MEASUREMENT_SIZE]  = {0};
void hts_temp_measurement_indicate_event(void)
{
	int err = 0;
	
	if (gatt_hts.temp_measurement_ccc && !htm_indicating) {
		int len = 0;
		u32_t temp_value = 0;
		memset(temp_measurement_buf, 0, sizeof(temp_measurement_buf));
		
		/* flag */
		if (gatt_hts.temp_measurement.temp_in_fahr_units) {
			temp_measurement_buf[len] |= (1 << HTS_FLAG_TEMPERATURE_UNITS);
		}
		if (gatt_hts.temp_measurement.temp_type_present) {
			temp_measurement_buf[len] |= (1 << HTS_FLAG_TEMPERATURE_TYPE);
		}
		if (gatt_hts.temp_measurement.time_stamp_present) {
			temp_measurement_buf[len] |= (1 << HTS_FLAG_TIME_STAMP);
		}
		len++;
		
		/* temperature */
		if (gatt_hts.temp_measurement.temp_in_fahr_units) {
			temp_value = (gatt_hts.temp_measurement.temp.fahrenheit.exponent << 24)
				| ((gatt_hts.temp_measurement.temp.fahrenheit.mantissa & 0x00ffffff) );
		} else {
			temp_value = (gatt_hts.temp_measurement.temp.celcius.exponent << 24) 
				| ((gatt_hts.temp_measurement.temp.celcius.mantissa & 0x00ffffff) );
		}
		len += u32_encode(temp_value, &temp_measurement_buf[len]);
		
		/* time stamp */
		if (gatt_hts.temp_measurement.time_stamp_present) {
			len += date_time_encode(&gatt_hts.temp_measurement.time_stamp, 
				&temp_measurement_buf[len]);
		}
		
		/* temperature type */
		if (gatt_hts.temp_measurement.temp_type_present) {
			temp_measurement_buf[len++] = gatt_hts.temp_type;
		}

		hts_temp_measurement_ind_params.attr = &hts_attrs[2];
		hts_temp_measurement_ind_params.func = hts_indicate_rsp;
		hts_temp_measurement_ind_params.data = &temp_measurement_buf;
		hts_temp_measurement_ind_params.len = len;

		if (!(err = bt_gatt_indicate(NULL, &hts_temp_measurement_ind_params))) {
			htm_indicating = 1;
		}
	}
}

void hts_intermediate_tmp_notify(void *data, int len)
{	
	bt_gatt_notify(NULL, &hts_attrs[7], data, len);
}

void hts_intermediate_tmp_notify_event(void)
{
	if (gatt_hts.intermediate_temp_ccc) {
		int len = 0;
		u32_t temp_value = 0;
		u8_t buf[HTS_TEMP_MEASUREMENT_SIZE]  = {0};

		/* flag */
		if (gatt_hts.temp_measurement.temp_in_fahr_units) {
			buf[len] |= (1 << HTS_FLAG_TEMPERATURE_UNITS);
		}
		if (gatt_hts.temp_measurement.temp_type_present) {
			buf[len] |= (1 << HTS_FLAG_TEMPERATURE_TYPE);
		}
		if (gatt_hts.temp_measurement.time_stamp_present) {
			buf[len] |= (1 << HTS_FLAG_TIME_STAMP);
		}
		len++;
		
		/* temperature */
		if (gatt_hts.temp_measurement.temp_in_fahr_units) {
			temp_value = (gatt_hts.temp_measurement.temp.fahrenheit.exponent << 24)
				| ((gatt_hts.temp_measurement.temp.fahrenheit.mantissa & 0x00ffffff) );
		} else {
			temp_value = (gatt_hts.temp_measurement.temp.celcius.exponent << 24) 
				| ((gatt_hts.temp_measurement.temp.celcius.mantissa & 0x00ffffff) );
		}

		len +=  u32_encode(temp_value, &buf[len]);

		/* time stamp */
		if (gatt_hts.temp_measurement.time_stamp_present) {
			len += date_time_encode(&gatt_hts.temp_measurement.time_stamp, 
				&buf[len]);
		}
		
		/* temperature type */
		if (gatt_hts.temp_measurement.temp_type_present) {
			buf[len++] = gatt_hts.temp_type;
		}
		
		hts_intermediate_tmp_notify(buf, len);
	}
}

void date_init(struct date_time *date_time)
{
	date_time->year = 2018;
	date_time->month = 3;
	date_time->day = 23;
	date_time->hours = 15;
	date_time->minutes = 43;
	date_time->seconds = 15;
}

void temperature_measurement_simulation()
{
	/* temperature */
	if (gatt_hts.temp_measurement.temp_in_fahr_units) {
		gatt_hts.temp_measurement.temp.fahrenheit.exponent = 0; 
		gatt_hts.temp_measurement.temp.fahrenheit.mantissa++;
		if (gatt_hts.temp_measurement.temp.fahrenheit.mantissa == 50) {
			gatt_hts.temp_measurement.temp.fahrenheit.mantissa = 30;
		}
	} else {
		gatt_hts.temp_measurement.temp.celcius.exponent = 0;  
		gatt_hts.temp_measurement.temp.celcius.mantissa++;
		if (gatt_hts.temp_measurement.temp.celcius.mantissa == 50) {
			gatt_hts.temp_measurement.temp.celcius.mantissa = 30;
		}
	}
	
	/* time stamp */
	if (gatt_hts.temp_measurement.time_stamp_present) {
		gatt_hts.temp_measurement.time_stamp = current_date;
	}
	
	/* temperature type */
	if (gatt_hts.temp_measurement.temp_type_present) {
		gatt_hts.temp_measurement.temp_type = gatt_hts.temp_type;
	}
}

void hts_timer_handler(struct k_timer *dummy)
{
	static u32_t systick = 0;

	systick++;
	/* temperature measurement */
	if (systick % 2 == 0) {
		temperature_measurement_simulation();
	}
	
	/* temperature measurement indicate */
	hts_temp_measurement_indicate_event();
	
	if (systick % 2 == 0) {
		/* intermediate temperature notify */
		hts_intermediate_tmp_notify_event();
	}	
}

void hts_timer_init(void)
{
	k_timer_init(&hts_timer, hts_timer_handler, NULL);
	k_timer_start(&hts_timer, K_SECONDS(1), K_SECONDS(1));
} 

void hts_init(void)
{
	memset(&gatt_hts, 0, sizeof(struct bt_gatt_hts));
	gatt_hts.temp_measurement.temp_in_fahr_units = 0;
	gatt_hts.temp_measurement.time_stamp_present = 0;
	gatt_hts.temp_measurement.temp_type_present = 1;
	gatt_hts.temp_type = HTS_TEMP_TYPE_BODY;
	
	date_init(&current_date);
	
	bt_gatt_service_register(&hts_svc);

	hts_timer_init();
}

