/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <soc.h>
#include "audio_in_acts.h"
#include "dma.h"
#include "soc_dma.h"

struct acts_audio_in_data audio_in_acts_data;

const struct acts_audio_in_config audio_in_acts_cfg = {
	.dma_id = DMA_ID_I2S,
	.dma_chan = 1,
	.hpfr_en  = 1,
	.hpfr_cfg = 3,
	.vdd_level = 6,
	.use_rx_clock = 0,
	.dc_cap_charge_wait_ms = 0,
};

void acts_audio_in_enable(struct device *dev, ain_setting_t *ain_setting, adc_setting_t *setting)
{
	audio_in_enable(dev, ain_setting, setting);
	device_busy_set(dev);
}

void acts_audio_in_disable(struct device *dev)
{
	audio_in_disable(dev);
	acts_reset_peripheral(RESET_ID_AUDIO);
	device_busy_clear(dev);
}
const struct audio_dev_driver_api audio_in_acts_driver_api = {
	.enable = acts_audio_in_enable,
	.disable = acts_audio_in_disable,
};

DEVICE_AND_API_INIT(audio_in, CONFIG_AUDIO_IN_ACTS_NAME, audio_in_init, &audio_in_acts_data, &audio_in_acts_cfg,
	POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &audio_in_acts_driver_api);
