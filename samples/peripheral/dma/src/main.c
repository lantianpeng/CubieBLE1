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
#include <dma.h>

#define DMA_DEVICE_NAME "DMA_0"
#define RX_BUFF_SIZE (48)

char tx_data[] = "It is harder to be kind than to be wise";
char rx_data[RX_BUFF_SIZE] = { 0 };

static void test_done(struct device *dev, u32_t id, int reason)
{
	if (reason == 0)
		printk("DMA transfer done\n");
	else if (reason == 1)
		printk("DMA half transfer done\n");
	else
		printk("DMA transfer met an error\n");
}

int test_dma(u32_t chan_id)
{
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};
	struct device *dma = device_get_binding(DMA_DEVICE_NAME);

	printk("test_task: chan %d\n", chan_id);

	if (!dma) {
		printk("Cannot get dma controller\n");
		return -1;
	}

	dma_cfg.channel_direction = MEMORY_TO_MEMORY;
	dma_cfg.source_data_size = 1;
	dma_cfg.dest_data_size = 1;
	dma_cfg.source_burst_length = 1;
	dma_cfg.dest_burst_length = 1;
	dma_cfg.dma_callback = test_done;
	dma_cfg.complete_callback_en = 1;
	dma_cfg.half_complete_callback_en = 1;
	dma_cfg.error_callback_en = 1;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_block_cfg;

	printk("Preparing DMA Controller: Chan_ID=%u\n",
			chan_id);

	printk("Starting the transfer\n");
	memset(rx_data, 0, sizeof(rx_data));
	dma_block_cfg.block_size = sizeof(tx_data);
	dma_block_cfg.source_address = (u32_t)tx_data;
	dma_block_cfg.dest_address = (u32_t)rx_data;

	if (dma_config(dma, chan_id, &dma_cfg)) {
		printk("ERROR: transfer\n");
		return -1;
	}

	if (dma_start(dma, chan_id)) {
		printk("ERROR: transfer\n");
		return -1;
	}
	k_sleep(5000);

	dma_stop(dma, chan_id);

	printk("rxdata: %s\n", rx_data);
	if (strcmp(tx_data, rx_data) != 0) {
		printk("compare error\n");
		return -1;
	}
	printk("compare pass\n");
	return 0;
}

void app_main(void)
{
	printk("\ndma testing\n");

	test_dma(0);

	while (1)
		;
}
