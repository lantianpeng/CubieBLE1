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
#include <gpio.h>

#define PIN_OUT 1  /* DIO1 */
#define PIN_IN 0  /* DIO0 */

void app_main(void)
{
	struct device *dev;
	u32_t val_write, val_read = 0;
	int i = 0;

	printk("\nGPIO testing\n");

	dev = device_get_binding(CONFIG_GPIO_ACTS_DRV_NAME);
	if (!dev) {
		printk("cannot found device \'%s\'\n",
			    CONFIG_GPIO_ACTS_DRV_NAME);
		goto out;
	}

	/* set PIN_OUT as writer */
	gpio_pin_configure(dev, PIN_OUT, GPIO_DIR_OUT);
	/* set PIN_IN as reader */
	gpio_pin_configure(dev, PIN_IN, GPIO_DIR_IN);
	gpio_pin_disable_callback(dev, PIN_IN);

	while (1) {
		val_write = i++ % 2;
		gpio_pin_write(dev, PIN_OUT, val_write);
		k_sleep(100);
		gpio_pin_read(dev, PIN_IN, &val_read);

		/*= checkpoint: compare write and read value =*/
		if (val_write != val_read)
			printk("Inconsistent GPIO read/write value\n");
		else
			printk("Consistent GPIO read/write value\n");
		}
out:
		while (1)
			;

}

