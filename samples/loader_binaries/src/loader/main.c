/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "boot.h"
#include "uart_adfu.h"
#include "system.h"
#include "string.h"

static int check_key(void)
{
	int key = KEY_APP; /* KEY_ADFU, KEY_DTM, KEY_OTA */

	/* get key(adc or matrix)*/
	return key;
}

struct partition_table part_table;

int get_partition_table(void)
{
	unsigned int partition_table_offset;
	unsigned int loader_phy_addr[2] = {0x0, 0x1000};
	char i;
	int ret = 0;

	for (i = 0; i < 2; i++) {
		partition_table_offset = loader_phy_addr[i] + PARTITION_TABLE_OFFSET;
		/* store partition_table_offset in RTC_BAK1 to help app getting the address easily */
		*(volatile unsigned int *)PARTITION_TABLE_ADDR = partition_table_offset;

		brom_spi0_read(partition_table_offset, sizeof(struct partition_table), (u8_t *)&part_table);

		if (part_table.magic != PARTITION_TABLE_MAGIC) {
			debug_uart_put_char('-');
			debug_uart_put_char('1');
			continue;
		}

#if 0
		cksum = crc32(&part_table, sizeof(struct partition_table) - 4, 0);
		if (cksum != part_table.table_crc) {
			debug_uart_put_char('-');
			debug_uart_put_char('2');
			continue;
		}
#endif

		break;
	}

	debug_uart_put_char('-');
	if (i == 0)
		debug_uart_put_char('F');
	else if (i == 1)
		debug_uart_put_char('S');
	else {
		debug_uart_put_char('E');
		ret = -1;
	}
	debug_uart_put_char('-');
	return ret;
}

struct partition_entry *find_part(char *name)
{
	struct partition_entry *part_entry;
	unsigned int i;

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part_entry = &part_table.parts[i];

		if (strcmp(name, (const char *)part_entry->name) == 0)
			return part_entry;
	}

	return 0;
}

struct partition_entry *find_part_by_type(char type)
{
	struct partition_entry *part_entry, *match_part_entry = NULL;
	unsigned int i;
	int seq = -1;

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part_entry = &part_table.parts[i];

		if (part_entry->type == type) {
			/* if many cheked image have the same type, select the image with large seq. */
			if (part_entry->seq > seq) {
				seq = part_entry->seq;
				match_part_entry = part_entry;
			}
		}
	}
	return match_part_entry;
}

int check_image_header(struct image_header *hdr)
{
	u32_t i, cksum;
	u32_t *pbuf = (u32_t *)hdr;

	if (hdr->magic != IMAGE_HEADER_MAGIC) {
		debug_uart_put_char('-');
		debug_uart_put_char('5');
		return -5;
	}

	cksum = 0x1234;
	for (i = 0; i < (sizeof(struct image_header)/4) - 1; i++)
		cksum += *(pbuf + i);

	if (cksum != hdr->hdr_checksum) {
		debug_uart_put_char('-');
		debug_uart_put_char('6');
		return -6;
	}

	return 0;
}

int check_image(struct image_header *hdr, unsigned char *data)
{
	u32_t i, cksum, bytes_len;
	u32_t *pbuf = (u32_t *)data;

	cksum = 0x1234;
	bytes_len = (hdr->rom_end - hdr->rom_start) / 4;

	for (i = 0; i < (IMAGE_HDR_OFFSET/4); i++)
		cksum += *(pbuf + i);

	for (i = (IMAGE_HDR_OFFSET + sizeof(struct image_header))/4; i < bytes_len; i++)
		cksum += *(pbuf + i);

	if (cksum != hdr->data_checksum) {
		debug_uart_put_char('-');
		debug_uart_put_char('7');
		return -7;
	}

	return 0;
}

int load_image(unsigned int nor_phy_addr, struct image_header *img_hdr)
{
	int ret;

	/* get image_header*/
	brom_spi0_read(nor_phy_addr + IMAGE_HDR_OFFSET, sizeof(struct image_header), (u8_t *)img_hdr);

	/* check header */
	ret = check_image_header(img_hdr);
	if (ret)
		goto exit;

	/* load image*/
	brom_spi0_read(nor_phy_addr, img_hdr->rom_end - img_hdr->rom_start, (u8_t *)img_hdr->rom_start);

	/* check image checksum */
	ret = check_image(img_hdr, (unsigned char *)img_hdr->rom_start);
	if (ret)
		goto exit;

exit:
	return ret;
}

int boot_app(unsigned int nor_phy_addr)
{
	struct image_header img_hdr;
	app_entry_t app_entry;
	int ret;

	ret = load_image(nor_phy_addr, &img_hdr);
	if (ret) {
		debug_uart_put_char('-');
		debug_uart_put_char('9');
		return -9;
	}

	debug_uart_put_char('b');

	app_entry = (app_entry_t)img_hdr.entry;
	app_entry();

	/* never run to here */
	return 0;
}

int boot_app_by_name(char *app_name)
{
	struct partition_entry *part;

	part = find_part(app_name);
	if (!part)
		return -8;

	return boot_app(part->offset);
}

int boot_app_by_type(char type)
{
	struct partition_entry *part;

	part = find_part_by_type(type);
	if (!part)
		return -8;

	return boot_app(part->offset);
}

int main(void)
{
	struct reboot_reason *reason;
	int ret;
	int keycode;

	/*  */
	debug_uart_put_char('C');
	debug_uart_put_char('U');
	debug_uart_put_char('A');
	check_uart_adfu();

	keycode = check_key();

	reason = (struct reboot_reason *)REBOOT_REASON_ADDR;
	if (reason->magic == REBOOT_REASON_MAGIC) {
		debug_uart_put_char('R');
		if (reason->reason == REBOOT_TYPE_GOTO_ADFU) {
			debug_uart_put_char('U');
			keycode = KEY_ADFU;
		} else if (reason->reason == REBOOT_TYPE_GOTO_DTM) {
			debug_uart_put_char('D');
			keycode = KEY_DTM;
		} else if (reason->reason == REBOOT_TYPE_GOTO_APP) {
			debug_uart_put_char('A');
			keycode = KEY_APP;
		}
	}
	/* invalidate reason magic */
	reason->magic = 0x55aa;

	if (get_partition_table())
		goto out;

	if (keycode == KEY_ADFU) {
		/* enter uart adfu */
	} else if (keycode == KEY_DTM) {
		/* load dtm */
		debug_uart_put_char('D');
		ret = boot_app_by_type(DTM_TYPE);
		if (ret)
			debug_uart_put_char('-');
	} else {
		/* load application*/
		debug_uart_put_char('A');
		ret = boot_app_by_type(SYSTEM_TYPE);
		if (ret)
			debug_uart_put_char('-');
	}

out:
	while (1)
		;
}
