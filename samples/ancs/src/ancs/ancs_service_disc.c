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
#include "app_disc.h"
#include "nvram_config.h"

/*============================================================================
 *  Private Data
 *============================================================================
 */

/* Discovered ANCS notification characteristic */
struct gatt_characteristic g_disc_ancs_char[] = {
	{
		INVALID_ATT_HANDLE,     /* start_handle */
		INVALID_ATT_HANDLE,     /* end_handle */
		{
			.u128.uuid.type = BT_UUID_TYPE_128,           /* UUID for ANCS notification characteristic */
			{
				0xBD, 0x1D, 0xA2, 0x99, 0xE6, 0x25, 0x58, 0x8C,
				0xD9, 0x42, 0x01, 0x63, 0x0D, 0x12, 0xBF, 0x9F
			},
		},

		true,                       /* Has_ccd */
		INVALID_ATT_HANDLE,         /* ccd_handle*/
		0 /*value*/
	},

	{
		INVALID_ATT_HANDLE,     /* start_handle */
		INVALID_ATT_HANDLE,     /* end_handle */
		{
			.u128.uuid.type = BT_UUID_TYPE_128,           /* UUID for ANCS control point characteristic */
			{
			0xD9, 0xD9, 0xAA, 0xFD, 0xBD, 0x9B, 0x21, 0x98,
			0xA8, 0x49, 0xE1, 0x45, 0xF3, 0xD8, 0xD1, 0x69
			},
		},

		false,                       /* Has_ccd */
		INVALID_ATT_HANDLE,         /* ccd_handle*/
		0 /*value*/
	},

	{
		INVALID_ATT_HANDLE,     /* start_handle */
		INVALID_ATT_HANDLE,     /* end_handle */
		{
			.u128.uuid.type = BT_UUID_TYPE_128,           /* UUID for ANCS data source characteristic */
			{
			0xFB, 0x7B, 0x7C, 0xCE, 0x6A, 0xB3, 0x44, 0xBE,
			0xB5, 0x4B, 0xD6, 0x24, 0xE9, 0xC6, 0xEA, 0x22
			},
		},

		true,                       /* Has_ccd */
		INVALID_ATT_HANDLE,         /* ccd_handle*/
		0 /*value*/
	},
};

/* Discovered ANCS Service  */
struct gatt_service g_disc_ancs_service = {
	INVALID_ATT_HANDLE,         /* start handle */
	INVALID_ATT_HANDLE,         /* end_handle */
	{
		.u128.uuid.type =  BT_UUID_TYPE_128,
		{
		0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4,
		0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79
		},
	},
	3,                          /* Number of characteristics  */
	0,                          /* characteristic index */
	g_disc_ancs_char,
	0,                          /* NVM_OFFSET*/
	read_disc_ancs_service_handles_from_nvm,
	write_disc_ancs_service_handles_to_nvm,
};


/*============================================================================
 *  Public Function Implementations
 *============================================================================
 */
extern int snprintf_rom(char *s, size_t len, const char *format, ...);
void write_disc_ancs_service_handles_to_nvm(void)
{
	u16_t index;

	/* Store the values in NVM */
	/* Write the service start handle */
	nvram_config_set("ancs_srv_s_h", (u16_t *)&g_disc_ancs_service.start_handle, sizeof(g_disc_ancs_service.start_handle));
	printk("service start_handle: 0x%x\n", g_disc_ancs_service.start_handle);

	/*Write the service end handle*/
	nvram_config_set("ancs_srv_e_h", (u16_t *)&g_disc_ancs_service.end_handle, sizeof(g_disc_ancs_service.end_handle));
	printk("service end_handle: 0x%x\n", g_disc_ancs_service.end_handle);

	/* Write the characteristic handles in NVM */
	for (index = 0 ; index < g_disc_ancs_service.num_chars ; index++) {
		/* Write the characteristic handle into NVM */
		char name[30];

		snprintf_rom(name, 30, "ancs_ch_h_%d", index);
		nvram_config_set(name, (u16_t *)&g_disc_ancs_char[index].start_handle, sizeof(g_disc_ancs_char[index].start_handle));
		printk("char[%d]: 0x%x\n", index, g_disc_ancs_char[index].start_handle);

		snprintf_rom(name, 30, "ancs_ch_ccd_h_%d", index);
		nvram_config_set(name, (u16_t *)&g_disc_ancs_char[index].ccd_handle, sizeof(g_disc_ancs_char[index].ccd_handle));
		printk("char_ccd[%d]: 0x%x\n", index, g_disc_ancs_char[index].ccd_handle);
	}

}

void read_disc_ancs_service_handles_from_nvm(bool handles_present)
{
	u16_t index;

	if (handles_present) {
		/* Read the service start handle from NVM */
		nvram_config_get("ancs_srv_s_h", (u16_t *)&g_disc_ancs_service.start_handle, sizeof(g_disc_ancs_service.start_handle));
		printk("service start_handle: 0x%x\n", g_disc_ancs_service.start_handle);

		/*Read the service end handle from NVM */
		nvram_config_get("ancs_srv_e_h", (u16_t *)&g_disc_ancs_service.end_handle, sizeof(g_disc_ancs_service.end_handle));
		printk("service end_handle: 0x%x\n", g_disc_ancs_service.end_handle);

		/*Read the characteristic handles from NVM */
		for (index = 0; index < g_disc_ancs_service.num_chars; index++) {
			char name[30];

			snprintf_rom(name, 30, "ancs_ch_h_%d", index);
			nvram_config_get(name, (u16_t *)&g_disc_ancs_char[index].start_handle, sizeof(g_disc_ancs_char[index].start_handle));
			printk("char[%d]: 0x%x\n", index, g_disc_ancs_char[index].start_handle);

			snprintf_rom(name, 30, "ancs_ch_ccd_h_%d", index);
			nvram_config_get(name, (u16_t *)&g_disc_ancs_char[index].ccd_handle, sizeof(g_disc_ancs_char[index].ccd_handle));
			printk("char_ccd[%d]: 0x%x\n", index, g_disc_ancs_char[index].ccd_handle);
		}
	}
}

u16_t get_remote_disc_ancs_service_start_handle(void)
{
	return g_disc_ancs_service.start_handle;
}

u16_t get_remote_disc_ancs_service_end_handle(void)
{
	return g_disc_ancs_service.end_handle;
}

bool does_handle_belong_to_discovered_ancs_service(u16_t handle)
{
	return ((handle >= g_disc_ancs_service.start_handle) &&
					(handle <= g_disc_ancs_service.end_handle))
					? true : false;
}

u16_t get_ancs_notification_handle(void)
{
	return g_disc_ancs_char[0].start_handle + 1;
}

u16_t get_ancs_notification_ccd_handle(void)
{
	return g_disc_ancs_char[0].ccd_handle;
}

u16_t get_ancs_control_point_handle(void)
{
	return g_disc_ancs_char[1].start_handle + 1;
}

u16_t get_ancs_data_source_handle(void)
{
	return g_disc_ancs_char[2].start_handle + 1;
}

u16_t get_ancs_data_source_ccd_handle(void)
{
	return g_disc_ancs_char[2].ccd_handle;
}
