/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPI driver for Actions SoC
 */

#define SYS_LOG_DOMAIN "SPI"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

#include "errno.h"
#include <kernel.h>
#include <soc.h>
#include <device.h>
#include <init.h>
#include <misc/util.h>
#include "spi_context.h"
#include "spi_acts.h"

__init_once_text int spi_acts_init_new(struct device *dev)
{
	const struct acts_spi_config *config = dev->config->config_info;
	struct acts_spi_data *data = dev->driver_data;

#ifdef CONFIG_SPI_ACTS_DMA
	data->dma_dev = device_get_binding(CONFIG_DMA_0_NAME);
	if (!data->dma_dev)
		return -ENODEV;

	k_sem_init(&data->dma_sync, 0, 1);
#endif

	/* enable spi controller clock */
	acts_clock_peripheral_enable(config->clock_id);

	/* reset spi controller */
	acts_reset_peripheral(config->reset_id);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_SPI_1
struct acts_spi_data spi_acts_data_port_1 = {
	SPI_CONTEXT_INIT_LOCK(spi_acts_data_port_1, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_acts_data_port_1, ctx),
};

const struct acts_spi_config spi_acts_cfg_1 = {
	.spi = (struct acts_spi_controller *) SPI1_REG_BASE,
	.spiclk_reg = CMU_SPI1CLK,
#ifdef CONFIG_SPI_ACTS_DMA
	.dma_chan0 = 1,
	.dma_chan1 = 2,
#else
	.dma_chan0 = -1,
	.dma_chan1 = -1,
#endif
	.txdma_id = DMA_ID_SPI1,
	.rxdma_id = DMA_ID_SPI1,

	.clock_id = CLOCK_ID_SPI1,
	.reset_id = RESET_ID_SPI1,
	.delay_chain = 8,
};

DEVICE_AND_API_INIT(spi_acts_1, CONFIG_SPI_1_NAME, spi_acts_init_new,
		    &spi_acts_data_port_1, &spi_acts_cfg_1,
		    POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
		    &acts_spi_driver_api);
#endif	/* CONFIG_SPI_1 */

#ifdef CONFIG_SPI_2
struct acts_spi_data spi_acts_data_port_2 = {
	SPI_CONTEXT_INIT_LOCK(spi_acts_data_port_2, ctx),
	SPI_CONTEXT_INIT_SYNC(spi_acts_data_port_2, ctx),
};

const struct acts_spi_config spi_acts_cfg_2 = {
	.spi = (struct acts_spi_controller *) SPI2_REG_BASE,
	.spiclk_reg = CMU_SPI2CLK,
#ifdef CONFIG_SPI_ACTS_DMA
	.dma_chan0 = 1,
	.dma_chan1 = 2,
#else
	.dma_chan0 = -1,
	.dma_chan1 = -1,
#endif
	.txdma_id = DMA_ID_SPI2,
	.rxdma_id = DMA_ID_SPI2,

	.clock_id = CLOCK_ID_SPI2,
	.reset_id = RESET_ID_SPI2,
	.delay_chain = 8,
};

DEVICE_AND_API_INIT(spi_acts_2, CONFIG_SPI_2_NAME, spi_acts_init_new,
		    &spi_acts_data_port_2, &spi_acts_cfg_2,
		    POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,
		    &acts_spi_driver_api);
#endif	/* CONFIG_SPI_2 */
