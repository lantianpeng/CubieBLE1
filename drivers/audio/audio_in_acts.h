/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @file
 *
 * @brief Actions Audio In header file
 */

#ifndef ACTS_AUDIO_IN_H_
#define ACTS_AUDIO_IN_H_

#include <zephyr/types.h>
#include <audio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct acts_audio_in_config {
	u8_t dma_id;
	s8_t dma_chan;
	u8_t hpfr_en;
	u8_t hpfr_cfg;
	u8_t vdd_level;
	u8_t use_rx_clock;
	u16_t dc_cap_charge_wait_ms;
};

struct acts_audio_in_data {
	struct device *dma_dev;
	u32_t backup_old_vdd;
};


extern int audio_in_init(struct device *dev);

extern void audio_in_enable(struct device *dev, ain_setting_t *ain_setting, adc_setting_t *adc_setting);

extern void audio_in_disable(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif  /*  ACTS_AUDIO_IN_H_ */
