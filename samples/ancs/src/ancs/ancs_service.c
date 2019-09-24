/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include "errno.h"
#include <zephyr.h>
#include <misc/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include "ancs_service.h"
#include "msg_manager.h"
#include "ancs_service_disc.h"
#include "app_ble.h"

/*============================================================================
 *  Private Definitions
 *===========================================================================
 */

/* Following macros define the offset of various fields in Notification Source
 * Characteristic.
 */
#define ANCS_NS_OFFSET_EVENT_ID                       (0)
#define ANCS_NS_OFFSET_EVENT_FLAGS                    (1)
#define ANCS_NS_OFFSET_CAT_ID                         (2)
#define ANCS_NS_OFFSET_CAT_COUNT                      (3)
#define ANCS_NS_OFFSET_NOTIF_UUID                     (4)

/* Following macro define the data source header length */
#define ANCS_DS_HDR_LEN                               (5)

/* Maximum Length of the notification attribute
 * Note: Do not increase length beyond (DEFAULT_ATT_MTU -3 = 20)
 * octets as GAP service at the moment doesn't support handling of Prepare
 * write and Execute write procedures.
 */
#define ANCS_MAX_NOTIF_ATT_LENGTH                     (20)

/*============================================================================
 *  Private Data Types
 *===========================================================================
 */

/* Structure used in decoding and displaying Attribute data
 */
struct ancs_attribute_data_t {
	/* Attribute length - used to know the length of the attribute length */
	u16_t attr_len;

	/* One more byte pending in the attribute length */
	bool pending_len;

	/* Attribute remaining/pending length - used to know the length of the
	 * remaining attribute length in the new data fragment
	 */
	u16_t rem_attr_len;

	/* Attribute data pending - used to know the length of the
	 * remaining attribute data in the new data fragment
	 */
	u16_t pending_attr_data;

	/* Attribute data state - used for decoding the attribute data
	 */
	u16_t ds_decoder_state;

 /* Used to identify the command processed */
	u16_t pending_cmd;

	/* Used to store the Attribute ID */
	u16_t attr_id;
};


/* Structure used for sending Get Notification Attribute request to peer*/
struct ancs_nofif_attr_cmd_t {
	/* Data to store and send the Notification
	 * attribute request
	 */
	u8_t data[ANCS_MAX_NOTIF_ATT_LENGTH + 1];
};


/* Structure used for sending Get Application Attribute request to peer */
struct ancs_app_attr_cmd_t {
	/* Data to store and send the Get App Attribute
	 * request
	 */
	u8_t data[(ANCS_MAX_NOTIF_ATT_LENGTH * 2) + 1];

};


/* Structure used for storing last UUID notification data */
struct ancs_notif_uuid_data_t {
	/* Array to store last UUID data */
	u8_t data[ANCS_NS_NOTIF_UUID_SIZE];

};

/*============================================================================
 *  Private Data
 *============================================================================
 */
/* Attribute data used for decoding and displaying peer notification data */
static struct ancs_attribute_data_t attribute_data;

/* Used for sending Get Notification Attribute request data
 */
static struct ancs_nofif_attr_cmd_t notif_attr_req;

/* Used for storing the last UUID received */
static struct ancs_notif_uuid_data_t uuid_data;

/* current connection identifier */
/* static u16_t g_cid; */

/* Buffer to hold the received ANCS data for display purpose */
u8_t g_ancs_data[ANCS_MAX_NOTIF_ATT_LENGTH + 1];

/* variable to track the last received notification */
u16_t g_last_received_notification;
/*=============================================================================
 *  Private Function Prototypes
 *============================================================================
 */

/* This function handles the NS notifications */
bool ancs_handle_notification_source_data(struct bt_conn *conn, const u8_t *notify_data, u16_t length);

/* This function handles the DS notifications */
bool ancs_handle_data_source_data(struct bt_conn *conn, const u8_t *notify_data, u16_t length);

/* This function parses and displays the data source & ANCS notification data */
static bool ancs_parse_data(u8_t *p_data, u16_t size_value);

/* This function initialises the Attribute data structure */
static void ancs_clear_attribute_data(void);


/*=============================================================================
 *  Private Function Implementations
 *============================================================================
 */

/**
 *  @brief
 *      ancs_parse_data
 *
 *
 *      This function parses and displays the DS notification data
 *
 *  @return
 *      true, if valid else false.
 *
 **/
static bool ancs_parse_data(u8_t *p_data, u16_t size_value)
{
	u16_t count = 0;
	u8_t attrId = 0;
	u16_t i = 0;
	u16_t state = attribute_data.ds_decoder_state;
	u16_t data_len = 0;
	bool b_skip_reserved = false;

	while (count < size_value) {
		if (b_skip_reserved)
			break;

		switch (state) {
		case ds_decoder_hdr:
			count = ANCS_DS_HDR_LEN;
			if (p_data[0] != 0) {
				printk("invalid command id\n");
				return false;
			}
			state = ds_decoder_attrid;
			break;

		case ds_decoder_attrid:
			/* Get the Attribute ID */
			attrId = p_data[count++];
			attribute_data.attr_id = attrId;

			printk("\r\n\r\n Attr ID = ");

			switch (attrId) {
			case ancs_notif_att_id_app_id:
				printk(" App ID");
				break;

			case ancs_notif_att_id_title:
				printk(" Title ");
				break;

			case ancs_notif_att_id_subtitle:
				printk(" Sub Title");
				break;

			case ancs_notif_att_id_message:
				printk(" Message");
				break;

			case ancs_notif_att_id_message_size:
				printk(" Message Size");
				break;

			case ancs_notif_att_id_date:
				printk(" Date");
				break;

			default:
				printk(" Reserved");
				b_skip_reserved = true;
				break;
			}

			if (!b_skip_reserved) {
				state = ds_decoder_attrlen;
			} else {
				state = ds_decoder_attrid;

				/* Invalid */
				attribute_data.attr_id = 0xff;
			}
			break;

		case ds_decoder_attrlen:

			if (attribute_data.pending_len) {
				/* Length was incomplete in the last fragment,
				 * so the first byte will complete the length
				 * data
				 */
				attribute_data.attr_len = ((p_data[count++] << 8) |
						 attribute_data.rem_attr_len);
				attribute_data.rem_attr_len = 0;
				attribute_data.pending_len = false;

			} else if ((count + 1) < size_value) {
				/* Get the Attribute length ( 2 bytes size) */
				attribute_data.attr_len = (p_data[count] |
													(p_data[count + 1] << 8));
				count += 2;
			} else {
				attribute_data.rem_attr_len = p_data[count++];
				/* Length is 2 bytes, so copy the byte and wait for
				 * the next byte in the new fragment
				 */
				attribute_data.pending_len = true;
			}

			if (!attribute_data.pending_len) {
				printk("\r\n Attr Len = 0x%02x", attribute_data.attr_len);

				if (attribute_data.attr_len > 0) {
					printk("\r\n Attribute Data = ");
				} else {
						printk("\r\n ");
				}
				state = ds_decoder_attrdata;
			}
			break;

		case ds_decoder_attrdata:
		{
			/* Data was incomplete in the last fragment */
			if (attribute_data.pending_attr_data) {
				/* Get the actual data length */
				data_len = size_value - count;


				/* If the received length is greater than
				 * the overall attribute length, just copy
				 * data till attribute length
				 */
				if (attribute_data.pending_attr_data < data_len) {
					data_len  = attribute_data.pending_attr_data;
				}

				/* Reset the g_ancs_data */
				memset(g_ancs_data, 0, ANCS_MAX_NOTIF_ATT_LENGTH + 1);

				/* Copy the data */
				for (i = 0; i < data_len; i++) {
					g_ancs_data[i] = p_data[count + i];
				}
				g_ancs_data[i] = '\0';

				/* Display to UART */
				printk((const char *)&g_ancs_data[0]);

				/* Update till, what we have read */
				count += data_len;
				attribute_data.pending_attr_data -= data_len;
			} else {
				if (attribute_data.attr_len > 0) {
					/* Get the actual data length */
					data_len = size_value - count;

					/* If the received length is greater than
					 * the overall attribute length, just copy
					 * data till attribute length
					 */
					if (attribute_data.pending_attr_data < data_len) {
						data_len  = attribute_data.pending_attr_data;
					}

					/* Reset the g_ancs_data */
					memset(g_ancs_data, 0, ANCS_MAX_NOTIF_ATT_LENGTH + 1);

					/* Copy the data */
					for (i = 0; i < data_len; i++) {
						g_ancs_data[i] = p_data[count + i];
					}

					g_ancs_data[i] = '\0';

					/* Display to UART */
					printk((const char *)&g_ancs_data[0]);

				 /* Is more data remaining? */
					attribute_data.pending_attr_data =
											(attribute_data.attr_len - data_len);
					attribute_data.attr_len = 0;
					count += data_len;
				}
			}

			if ((attribute_data.pending_attr_data == 0) &&
						(attribute_data.attr_len == 0)) {
				/* We are done reading data.Move to next attribute */
				state = ds_decoder_attrid;
				/* Invalid */
				attribute_data.attr_id = 0xff;
			}
			break;
			}
		}
	}
	attribute_data.ds_decoder_state = state;

	return true;
}

/**
 *  @brief
 *      ancs_handle_notification_source_data
 *
 *
 *      This function handles the NS notifications
 *
 *  @return
 *      true, if valid else false.
 *
 */
bool ancs_handle_notification_source_data(struct bt_conn *conn, const u8_t *notify_data, u16_t length)
{
	u16_t curr_data = 0;
	u16_t count = 0;
	bool notif_removed = false;

	/*  Notification Source Data format
	 * -------------------------------------------------------
	 * |  Event  |  Event  |  Cat  |  Cat   |  Notification  |
	 * |  ID     |  Flag   |  ID   |  Count |  UUID          |
	 * |---------------------------------------------------- |
	 * |   1B    |   1B    |   1B  |   1B   |   4B           |
	 * -------------------------------------------------------
	 */

	if (notify_data != NULL) {
		printk("\r\n");
		printk("\r\n Event ID = ");
		/* 1st byte of the Notification - Event ID */
		curr_data = notify_data[ANCS_NS_OFFSET_EVENT_ID];
		if (curr_data == ancs_event_id_notif_added)
			printk(" Added");

		else if (curr_data == ancs_event_id_notif_modified) {
			printk(" Modified");
		} else if (curr_data == ancs_event_id_notif_removed) {
			notif_removed = true;
			printk(" Removed");
		} else
			printk(" Reserved");


		printk("\r\n Event Flags = ");

		/* 2nd byte of the Notification- Event Flags */
		curr_data = notify_data[ANCS_NS_OFFSET_EVENT_FLAGS];

		if (curr_data == ANCS_NS_EVENTFLAG_SILENT)
			printk("Silent");
		else if (curr_data == ANCS_NS_EVENTFLAG_IMPORTANT)
			printk("Important");
		else /* Reserved */
			printk("Reserved");


		printk("\r\n Cat ID = ");

		/* 3rd byte of the Notification - Cat ID */
		curr_data = notify_data[ANCS_NS_OFFSET_CAT_ID];

		switch (curr_data) {
		case ancs_cat_id_other:
				printk("Other");
				break;

		case ancs_cat_id_incoming_call:
				printk("Incoming call");
				break;

		case ancs_cat_id_missed_call:
				printk("Missed call");
				break;

		case ancs_cat_id_vmail:
				printk("vmail");
				break;

		case ancs_cat_id_social:
				printk("social");
				break;

		case ancs_cat_id_schedule:
				printk("schedule");
				break;

		case ancs_cat_id_email:
				printk("email");
				break;

		case ancs_cat_id_news:
				printk("news");
				break;

		case ancs_cat_id_hnf:
				printk("hnf");
				break;

		case ancs_cat_id_bnf:
				printk("bnf");
				break;

		case ancs_cat_id_location:
				printk("location");
				break;

		case ancs_cat_id_entertainment:
				printk("entertainment");
				break;
		default:
				printk("reserved");
				break;
		}

		/* 4rd byte of the Notification - Cat Count */
		printk("\r\n Cat Count = %d", notify_data[ANCS_NS_OFFSET_CAT_COUNT]);

		if (!notif_removed) {
			/* 5th to 8th bytes (4 bytes) of the Notification-Notification UUID */
			printk("\r\n UUID = ");

			/* Clear the UUID notification buffer */
			memset(uuid_data.data, 0, ANCS_NS_NOTIF_UUID_SIZE);

			for (count = 0; count < ANCS_NS_NOTIF_UUID_SIZE; count++) {
				printk("%d", notify_data[ANCS_NS_OFFSET_NOTIF_UUID + count]);
				uuid_data.data[count] = notify_data[ANCS_NS_OFFSET_NOTIF_UUID+count];
			}
			printk("\r\n");

			g_last_received_notification = curr_data;

			/* Send Notification Attribute Request */
			{
				struct app_msg msg = {0};

				printk("ancs_get_notification_attribute_cmd\n");
				/* send a message to main thread */
				msg.type = MSG_ANCS_SERVICE;
				msg.value = (int)conn;
				send_msg(&msg, K_MSEC(1000));
			}
		}
	}

	return true;
}

/**
 *  @brief
 *      ancs_handle_data_source_data
 *
 *
 *      This function handles the DS notifications
 *
 *  @return
 *      true, if valid else false.
 *
 **/
bool ancs_handle_data_source_data(struct bt_conn *conn, const u8_t *notify_data, u16_t length)
{
	u16_t count = 0;
	u16_t state = attribute_data.ds_decoder_state;

	/* if cmd id = 1*/
	/* parse the data */
	if (attribute_data.pending_cmd == ancs_cmd_get_notification_att) {
		if (((notify_data[count] == ancs_cmd_get_notification_att) &&
			 (attribute_data.pending_attr_data == 0) &&
			 (attribute_data.attr_len == 0) && (!attribute_data.pending_len))) {
			if ((state == ds_decoder_hdr) || (state == ds_decoder_attrid)) {
				attribute_data.ds_decoder_state = ds_decoder_hdr;
			}
		}
		ancs_parse_data((u8_t *)notify_data, length);
	} else {
		/* if cmd id = 1*/
		/* Check for APP ID */
		printk("\r\nDisplay Name ");
		printk((char *)notify_data + 1);
		printk("\r\n");
	}
	return true;
}


/**
 *  @brief
 *      ancsCheckCharUuid
 *
 *
 *      This function checks if the UUID provided is one of the ANCS
 *      Characteristics
 *  @return
 *       ancs_type
 *
 **/

static void ancs_clear_attribute_data(void)
{
	/* Reset the attribute data */
	attribute_data.ds_decoder_state = ds_decoder_hdr;
	attribute_data.attr_len = 0;
	attribute_data.pending_len = false;
	attribute_data.rem_attr_len = 0;
	attribute_data.pending_attr_data = 0;
}

/*============================================================================
 *  Public Function Implementations
 *===========================================================================
 */

extern void ancs_service_data_init(void)
{
	/* Reset the Attribute data used */
	ancs_clear_attribute_data();
}


extern void ancs_get_notification_attribute_cmd(struct bt_conn *conn)
{
	u16_t count = 0;
	u16_t loop_count = 0;
	u8_t *value = NULL;
	/* "ancs_notif_att_id_app_id" is not added as it is sent by default by the IOS
	 * device
	 */
	u16_t features = (ancs_notif_att_id_title|ancs_notif_att_id_subtitle |
										ancs_notif_att_id_message|ancs_notif_att_id_message_size|
										ancs_notif_att_id_date);

	/* Clear the buffer each time */
	memset(notif_attr_req.data, 0, ANCS_MAX_NOTIF_ATT_LENGTH + 1);

	value = &notif_attr_req.data[0];

	/* Fill in the command -id (0) for the Notification Attribute request */
	value[count++] = ancs_cmd_get_notification_att;

	/* Copy the Notification UUID */
	for (loop_count = 0; loop_count < ANCS_NS_NOTIF_UUID_SIZE; loop_count++)
		value[loop_count + count] = uuid_data.data[loop_count];

	/* Add the length of UUID */
	count += ANCS_NS_NOTIF_UUID_SIZE;

	/* "ancs_notif_att_id_app_id" is sent by default, so no need to request the
	 * attribute separately. Attribute App Identifier has no length to be filled
	 * in
	 */
	/* Add Attribute ID for Title */
	value[count++] = ancs_notif_att_id_title;

	/* Add Attribute size for Title - 0x14 bytes requested */
	value[count++] = 0x14;
	value[count++] = 0;


	if (features & ancs_notif_att_id_subtitle) {
		/* Add Attribute ID for Sub title */
		value[count++] = ancs_notif_att_id_subtitle;

		/* Attribute size - 0x14 bytes requested */
		value[count++] = 0x14;
		value[count++] = 0;
	}

	if (features & ancs_notif_att_id_message) {
		/* Add Attribute ID for Message */
		value[count++] = ancs_notif_att_id_message;
	}

	if (features & ancs_notif_att_id_message_size) {
		/* Add Attribute ID for Message size */
		value[count++] = ancs_notif_att_id_message_size;
	}

	/* Add Attribute ID - Date is sent as UTF-35# support
	 *Date format : YYYYMMDD'T'HHMMSS
	 */
	value[count++] = ancs_notif_att_id_date;

	/* Notification Attribute Request format
	 * -------------------------------------------------------------------------
	 * |  CMD    |  Notification  |  Attr    |  Attr  |   Attr  |  Attr   | Attr|
	 * |  ID(0)  |  UUID          |  ID-1    |  ID-2  |   Len   |  ID-n   | Len |
	 * |         |                | (App ID) |        |         |         |     |
	 * |------------------------------------------------------------------------
	 * |   1B    |      4B        |   1B     |   1B   |   2B    |    1B   |  2B |
	 * --------------------------------------------------------------------------
	 */

	ancs_write_request(ancs_cmd_get_notification_att, &notif_attr_req.data[0], count, conn);

	/* Set the command requested for */
	attribute_data.pending_cmd = ancs_cmd_get_notification_att;
}
static struct bt_gatt_subscribe_params subscribe_params_ns;
static struct bt_gatt_subscribe_params subscribe_params_nc;
static u8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, u16_t length)
{
	if (!data) {
		printk("[UNSUBSCRIBED]\n");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	printk("[NOTIFICATION] handle %d data %p length %u\n", params->value_handle, data, length);

	if (get_ancs_notification_handle() == params->value_handle) {
		ancs_handle_notification_source_data(conn, data, length);
	} else if (params->value_handle == get_ancs_data_source_handle()) {
		ancs_handle_data_source_data(conn, data, length);
	}

	return BT_GATT_ITER_CONTINUE;
}

extern bool configure_ancs_notifications(struct bt_conn *conn)
{
	int err;

	subscribe_params_ns.notify = notify_func;
	subscribe_params_ns.value = BT_GATT_CCC_NOTIFY;
	subscribe_params_ns.ccc_handle = get_ancs_notification_ccd_handle();
	subscribe_params_ns.value_handle = get_ancs_notification_handle();
	subscribe_params_ns.flags |= BT_GATT_SUBSCRIBE_FLAG_VOLATILE;
	printk("subscribe_params_ns.ccc_handle 0x%2x, 0x%2x\n", subscribe_params_ns.ccc_handle, subscribe_params_ns.value_handle);
	err = bt_gatt_subscribe(conn, &subscribe_params_ns);
	if (err && err != -EALREADY) {
		printk("Subscribe failed (err %d)\n", err);
	} else {
		printk("[SUBSCRIBED1]\n");
	}
	return err;
}


extern bool configure_ancs_data_source_notification(struct bt_conn *conn)
{
	int err;

	subscribe_params_nc.notify = notify_func;
	subscribe_params_nc.value = BT_GATT_CCC_NOTIFY;
	subscribe_params_nc.ccc_handle = get_ancs_data_source_ccd_handle();
	subscribe_params_nc.value_handle = get_ancs_data_source_handle();
	subscribe_params_nc.flags |= BT_GATT_SUBSCRIBE_FLAG_VOLATILE;
	printk("subscribe_params_nc.ccc_handle 0x%2x, 0x%2x\n", subscribe_params_nc.ccc_handle, subscribe_params_nc.value_handle);
	err = bt_gatt_subscribe(conn, &subscribe_params_nc);
	if (err && err != -EALREADY) {
		printk("Subscribe failed (err %d)\n", err);
	} else {
		printk("[SUBSCRIBED]\n");
	}
	return err;
}


bool ancs_handler_notif_ind(struct bt_conn *conn, u16_t handle, const u8_t *notify_data, u16_t length)
{
	if (handle == get_ancs_notification_handle()) {  /* Notification has arrived */
		ancs_handle_notification_source_data(conn, notify_data, length);
		return true;
	} else if (handle == get_ancs_data_source_handle()) {
	/* Detailed data about the previous notification has arrived */
	ancs_handle_data_source_data(conn, notify_data, length);
	return false;
	} else {
		return false;
	}
}

bool does_handle_belongs_to_ancs_service(u16_t handle)
{
	return ((handle >= get_remote_disc_ancs_service_start_handle()) &&
					(handle <= get_remote_disc_ancs_service_end_handle()))
					? true : false;
}

static struct bt_gatt_write_params write_params;

u8_t pending_flag;
static void write_func(struct bt_conn *conn, u8_t err,
		       struct bt_gatt_write_params *params)
{
	printk("Write complete: err %u\n", err);
	pending_flag = 0;
	memset(&write_params, 0, sizeof(write_params));
}

bool ancs_write_request(u16_t type, u8_t *data, u16_t size, struct bt_conn *conn)
{
	write_params.data = data;
	write_params.length = size;
	write_params.handle = get_ancs_control_point_handle();
	write_params.offset = 0;
	write_params.func = write_func;

	printk("write handle : 0x%x\n", get_ancs_control_point_handle());
	if (!pending_flag) {
		if (bt_gatt_write(conn, &write_params) == 0) {
			pending_flag = 1;
		}
	}
	return true;
}
