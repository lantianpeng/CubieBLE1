/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief dma controller driver for Actions SoC
 */

#include <kernel.h>
#include <init.h>
#include <dma.h>
#include <soc.h>
#include "dma_acts.h"

const struct acts_dma_config dma_acts_cdata = {
	.config = dma_acts_init_config,
};

static struct acts_dma_device dma_acts_ddata;

DEVICE_AND_API_INIT(dma_acts, CONFIG_DMA_0_NAME, dma_acts_init,
		    &dma_acts_ddata, &dma_acts_cdata,
		    PRE_KERNEL_1, CONFIG_DMA_ACTS_DEVICE_INIT_PRIORITY,
		    &dma_acts_driver_api);

static __init_once_text void dma_acts_init_config(struct acts_dma_device *ddev)
{
	IRQ_CONNECT(IRQ_ID_DMA, CONFIG_DMA_0_IRQ_PRI,
		    dma_acts_irq, DEVICE_GET(dma_acts), 0);
	irq_enable(IRQ_ID_DMA);
}
