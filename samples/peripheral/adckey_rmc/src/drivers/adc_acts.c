#if CONFIG_DEEPSLEEP
/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC driver for Actions SoC
 */
#include <kernel.h>
#include <init.h>
#include <soc.h>
#include "adc_acts.h"
#include <misc/byteorder.h>

#define CHAN_DATA_REG(base, chan)	((volatile u32_t)(base) + 0x14 + (chan) * 0x4)
	
/* dma channel registers */
struct acts_adc_controller {
	volatile u32_t ctrl0;
	volatile u32_t ctrl1;
	volatile u32_t ctrl2;
	volatile u32_t int_ctrl;
	volatile u32_t int_pd;
};

int adc_acts_read_new(struct device *dev, struct adc_seq_table *seq_tbl)
{
	const struct acts_adc_config *cfg = dev->config->config_info;
	struct acts_adc_controller *adc = (struct acts_adc_controller *)cfg->base;
	struct adc_seq_entry *entry;
	u32_t key;
	int i, chan_id;
	u16_t adc_val;

	for (i = 0; i < seq_tbl->num_entries; i++) {
		entry = &seq_tbl->entries[i];
		if (!entry->buffer)
			break;

		chan_id = entry->channel_id;
		if (chan_id > ADC_ID_MAX_ID)
			break;

		/* enable adc channel */
		key = irq_lock();
		adc->int_pd = BIT(chan_id);
		adc->ctrl0 |= BIT(chan_id);
		irq_unlock(key);

		while((adc->int_pd & BIT(chan_id)) == 0);

		adc_val = sys_read32(CHAN_DATA_REG(adc, chan_id));
		sys_put_le16(adc_val, entry->buffer);

		/* disable adc channel */
		key = irq_lock();
		//adc->ctrl0 &= ~BIT(chan_id);
		adc->int_pd = BIT(chan_id);
		irq_unlock(key);
	}

	return 0;
}


struct acts_adc_data adc_acts_ddata;

const struct acts_adc_config adc_acts_cdata = {
	.base = ADC_REG_BASE,
	.clock_id = CLOCK_ID_LRADC,
};

extern void adc_acts_enable(struct device *dev);
extern void adc_acts_disable(struct device *dev);

const struct adc_driver_api adc_acts_driver_api_new = {
	.enable = adc_acts_enable,
	.disable = adc_acts_disable,
	.read = adc_acts_read_new,
};

DEVICE_AND_API_INIT(adc_acts, CONFIG_ADC_0_NAME, adc_acts_init,
		    &adc_acts_ddata, &adc_acts_cdata,
		    PRE_KERNEL_2, CONFIG_ADC_INIT_PRIORITY,
		    &adc_acts_driver_api_new);

#else  /**********************************************************************************************************************************/

/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC driver for Actions SoC
 */
#include <kernel.h>
#include <init.h>
#include <soc.h>
#include "adc_acts.h"

struct acts_adc_data adc_acts_ddata;

const struct acts_adc_config adc_acts_cdata = {
	.base = ADC_REG_BASE,
	.clock_id = CLOCK_ID_LRADC,
};

DEVICE_AND_API_INIT(adc_acts, CONFIG_ADC_0_NAME, adc_acts_init,
		    &adc_acts_ddata, &adc_acts_cdata,
		    PRE_KERNEL_2, CONFIG_ADC_INIT_PRIORITY,
		    &adc_acts_driver_api);

#endif
