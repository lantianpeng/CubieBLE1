/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for adc key
 */

#ifndef __DMA_ACTS_H__
#define __DMA_ACTS_H__

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct acts_dma_chan {
	bool busy;
	void (*dma_callback)(struct device *dev, u32_t id,
			     int error_code);
	void *callback_data;

	u32_t  complete_callback_en		: 1;
	u32_t  half_complete_callback_en	: 1;
	u32_t  error_callback_en		: 1;
	u32_t  reserved				: 29;
};

struct acts_dma_device {
	void *base;
	struct acts_dma_chan chan[DMA_ACTS_MAX_CHANNELS];
};

struct acts_dma_config {
	void (*config)(struct acts_dma_device *);
};

extern const struct dma_driver_api dma_acts_driver_api;

int dma_acts_init(struct device *dev);
void dma_acts_irq(struct device *dev);

void dma_acts_init_config(struct acts_dma_device *ddev);

#ifdef __cplusplus
}
#endif

#endif	/* __DMA_ACTS_H__ */
