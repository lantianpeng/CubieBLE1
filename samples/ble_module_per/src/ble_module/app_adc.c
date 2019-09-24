/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <misc/printk.h>
#include "adc.h"
#include "soc_adc.h"

#define SYS_LOG_DOMAIN "battery"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>


u32_t adc_get(u8_t channel_id)
{
		struct device *adc_dev;
		struct adc_seq_table seq_tbl;
		struct adc_seq_entry seq_entrys;
		u8_t adcval[4];
		u16_t adc_val;

		adc_dev = device_get_binding(CONFIG_ADC_0_NAME);
		if (!adc_dev) {
			SYS_LOG_ERR("cannot found adc dev");
			return 0;
		}

		seq_entrys.sampling_delay = 0;
		seq_entrys.buffer = (u8_t *)&adcval;
		seq_entrys.buffer_length = sizeof(adcval);
		seq_entrys.channel_id = channel_id;

		seq_tbl.entries = &seq_entrys;
		seq_tbl.num_entries = 1;

		adc_enable(adc_dev);

		/* get adc value */
		adc_read(adc_dev, &seq_tbl);
		if (channel_id == ADC_ID_BATV) {
			adc_val = (seq_entrys.buffer[0] + (seq_entrys.buffer[1] << 8)) * 3600 / 1024; /* mv */
		} else {
			adc_val = (seq_entrys.buffer[0] + (seq_entrys.buffer[1] << 8));
		}

		printk("adcval %d\n", adc_val);

		adc_disable(adc_dev);

		return adc_val;
}

extern void ble_send_drv_data(u8_t *data, u16_t len);
void adc_event_handle(u32_t event)
{
	u32_t val;
	u8_t tmp_buf[2];
	switch (event) {
	case 1:
		val = adc_get(ADC_ID_BATV);
		tmp_buf[0] = (val/1000) & 0xFF;
		tmp_buf[1] = ((val - (val/1000*1000)) /10) & 0xFF;
		ble_send_drv_data(tmp_buf,sizeof(tmp_buf));
		break;
	case 2:
		val = adc_get(ADC_ID_CH0 + MODULE_ADC0_PIN);
		tmp_buf[0] = (val/1000) & 0xFF;
		tmp_buf[1] = ((val - (val/1000*1000)) /10) & 0xFF;
		ble_send_drv_data(tmp_buf,sizeof(tmp_buf));
		break;
	}
}
