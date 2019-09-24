/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <zephyr.h>
#include <shell/shell.h>
#include <input_dev.h>
#include <flash.h>

#include "system_app_storage.h"
//#ifdef	CONFIG_AUDIO_OUT
struct device *flash_dev;

/* max audio file size */
u32_t audio_file_size = 1024 * (8 * 2) / 4 * 30; /* 30 s  8k audio */

/* start addr of app storage */
u32_t audio_file_addr = 128 * 1024;

/* storage size of app storage */
#define APP_STROAGE_SIZE 1024 * 256

void write_data_to_storage(u8_t *buf, u16_t len)
{	
	if(flash_dev) 
	{
		flash_write(flash_dev, (off_t)((audio_file_addr + audio_file_size)&0xFFFFF), buf, len);
		audio_file_size += len;
		printk("audio_file_size is 0x%x\n",audio_file_size);
		//printk("flash write data %d to 0x%x\n",len, (off_t)(audio_file_addr + audio_file_size));
	}
}

void reset_stroage(void)
{
		audio_file_size = 0;
		if(flash_dev) 
		{
			flash_erase(flash_dev, (off_t)(audio_file_addr&0xFFFFF), APP_STROAGE_SIZE);
			printk("erase flash for audio\n");
		}
}

__init_once_text void system_app_storage_init(void)
{
	flash_dev = device_get_binding(CONFIG_NVRAM_STORAGE_DEV_NAME);
	if (!flash_dev) {
		printk("cannot found device \'%s\'\n",CONFIG_NVRAM_STORAGE_DEV_NAME);
	}
	printk("flash dev init\n");
}
//#endif
