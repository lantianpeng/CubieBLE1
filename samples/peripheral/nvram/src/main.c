/* main.c - Application main entry point */

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

static unsigned char test_nvram_buf[512 * 2];

#include <nvram_config.h>

void read_config(const char *name)
{
	int data_len;

	test_nvram_buf[0] = 0;
	data_len = nvram_config_get(name, test_nvram_buf, 512);
	if (data_len >= 0)
		printk("data_len %d, %s: %s\n", data_len, name, test_nvram_buf);
	else
		printk("cannot find config %s, ret %d\n", name, data_len);
}

void read_factory_config(const char *name)
{
	int data_len;

	test_nvram_buf[0] = 0;
	data_len = nvram_config_get_factory(name, test_nvram_buf, 512);
	if (data_len >= 0)
		printk("data_len %d, %s: %s\n", data_len, name, test_nvram_buf);
	else
		printk("cannot find config %s, ret %d\n", name, data_len);
}

int write_config(const char *name, char *data)
{
	int data_len, ret;

	data_len = strlen(data) + 1;

	ret = nvram_config_set(name, data, data_len);
	if (ret) {
		printk("failed to set cfg %s\n", name);
		return ret;
	}

	return 0;
}

int write_factory_config(const char *name, char *data)
{
	int data_len, ret;

	data_len = strlen(data) + 1;

	ret = nvram_config_set_factory(name, data, data_len);
	if (ret) {
		printk("failed to set cfg %s\n", name);
		return ret;
	}

	return 0;
}


void app_main(void)
{
	printk("\nNVRAM testing\n");

	nvram_config_dump();

	write_config("ucfg1", "udata1");
	write_factory_config("fcfg1", "fdata1");

	read_factory_config("fcfg1");
	read_config("fcfg1");
	read_config("ucfg1");

	nvram_config_dump();

	while (1)
		;
}
