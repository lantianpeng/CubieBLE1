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
#include <i2c.h>

#define E2PROM_I2C_MASTER_NAME  CONFIG_I2C_0_NAME
#define E2PROM_I2C_ADDRESS  0x50

#define TFER_COUNT 32

static const union dev_config i2c_cfg = {
	.raw = 0,
	.bits = {
		.use_10_bit_addr = 0,
		.is_master_device = 1,
		.speed = I2C_SPEED_STANDARD,
	},
};

void app_main(void)
{
	struct device *dev;
	int err;
	u16_t offset;
	u8_t i, wdata[TFER_COUNT], rdata[TFER_COUNT];

	printk("E2PROM test start!\n");

	dev = device_get_binding(E2PROM_I2C_MASTER_NAME);
	if (!dev) {
		printk("Cannot get I2C device");
		goto out;
	}

	err = i2c_configure(dev, i2c_cfg.raw);
	if (err) {
		printk("I2C config failed\n");
		goto out;
	}

	offset = 0;

	for (i = 0; i < TFER_COUNT; i++)
	wdata[i] = i;
	memset(rdata, 0, TFER_COUNT);

	/* write to E2PROM */
	err = i2c_burst_write16(dev, E2PROM_I2C_ADDRESS, offset, wdata, TFER_COUNT);
	if (err) {
		printk("E2PROM test write fail!\n");
		goto out;
	}
	/* delay 10ms before next E2PROM operation*/
	k_sleep(10);

	/* read from E2PROM */
	err = i2c_burst_read16(dev, E2PROM_I2C_ADDRESS, offset, rdata, TFER_COUNT);
	if (err) {
		printk("E2PROM test read fail!\n");
		goto out;
	}

	for (i = 0; i < TFER_COUNT; i++) {
		if (rdata[i] != wdata[i]) {
			printk("E2PROM Data compare error\n");
			goto out;
		}
	}

	printk("E2PROM test pass!\n");

out:
	while (1)
		;
}
