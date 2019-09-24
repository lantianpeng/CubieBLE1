/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef	_ACTIONS_BOOT_H_
#define	_ACTIONS_BOOT_H_

#include "types.h"

#ifndef _ASMLANGUAGE

#define PARTITION_TABLE_ADDR    0x40004034  /* RTC BAK1 */
#define REBOOT_REASON_ADDR		0x40004038  /* RTC BAK2 & RTC BAK3 */
#define REBOOT_REASON_MAGIC     0x544f4252

#define REBOOT_TYPE_NORMAL		0x0
#define REBOOT_TYPE_GOTO_ADFU 0x1000
#define REBOOT_TYPE_GOTO_DTM  0x1200
#define REBOOT_TYPE_GOTO_OTA	0x1300
#define REBOOT_TYPE_GOTO_APP  0x1400

struct reboot_reason {
	unsigned int magic;
	unsigned int reason;
};

#define KEY_ADFU      0xf0
#define KEY_OTA       0xf1
#define KEY_DTM       0xf2
#define KEY_APP       0xf3
#define KEY_RESERVED	0xff

#define MBRC_OFFSET (0x200000d0)

#define IMAGE_HDR_OFFSET  0xc0

/* 'A','T','B',0 */
#define IMAGE_HEADER_MAGIC 0x00425441

struct image_header {
	u32_t magic;  /*!< 'A','T','B',0 */
	u32_t rom_start;  /*!< vaddr start. */
	u32_t rom_end;  /*!< vaddr end. */
	u32_t entry;  /*!< vaddr entry. */
	u8_t reserve[8];
	u32_t data_checksum;  /*!< Checksum of data*/
	u32_t hdr_checksum;  /*!< Checksum over header*/
};


#define PARTITION_TABLE_OFFSET 0xc00

enum {
	RESERVE_TYPE = 0,
	BOOT_TYPE = 1,
	SYSTEM_TYPE = 2,
	RECOVREY_TYPE = 3,
	DATA_TYPE = 4,
	DTM_TYPE = 5,
};

/* Patition Addr */
#define LOADER_A_NOR_ADDR       0x0     /* offset: 0KB, size: 4 */
#define LOADER_B_NOR_ADDR       0x1000  /* offset: 4KB, size: 4 */
#define APP_A_NOR_ADDR          0x10000  /* offset: 64KB, size: 128KB */
#define APP_B_NOR_ADDR          0x30000  /* offset: 192KB, size: 128KB */
#define DTM_NOR_ADDR            0x50000   /* offset: 320KB, size: 128KB */
#define NVRAM_FACTORY_NOR_ADDR  0x70000   /* offset: 448KB, size: 4KB */
#define NVRAM_USER_NOR_ADDR     0x71000   /* offset: 452KB, size: 4KB */



#define PARTITION_TABLE_MAGIC		0x54504341	/* 'ACPT' */

/* fw0_boot: 01/01, 00000000, 00000000, 00000000 */
/* fw1_boot: 01/01, 00001000, 00000000, 00001000 */
/* img_dtm : 02/01, 00010000, 00000000, 00010000 */
/* img_ota : 03/01, 00020000, 00000000, 00020000 */
/* img_app : 02/01, 00030000, 00000000, 00030000 */
/* nv_fact : 04/00, 0007a000, 00000000, 0007a000 */
/* nv_user : 04/00, 0007c000, 00000000, 0007c000 */
struct partition_entry {
	u8_t name[8];
	u16_t type;
	u16_t flag;
	u32_t offset;
	s16_t seq;
	u16_t reserve;
	u32_t entry_offs;
};

#define MAX_PARTITION_COUNT	15

/* mbrc 4KB,partition_table located at offset 3KB */
struct partition_table {
	u32_t magic;
	u16_t version;
	u16_t table_size;
	u16_t part_cnt;
	u16_t part_entry_size;
	u8_t reserved1[4];

	/* 360Bytes */
	struct partition_entry parts[MAX_PARTITION_COUNT];
	u8_t Reserved2[4];
	u32_t table_crc;
};

extern struct partition_table part_table;

typedef void (*app_entry_t)(void);
extern u32_t brom_spi0_read(u32_t snor_addr, u32_t byte_len, u8_t *buf);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_BOOT_H_	*/
