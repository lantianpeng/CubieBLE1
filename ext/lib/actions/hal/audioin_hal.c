/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <audio.h>

#include "audioin_hal.h"
#include <net/buf.h>
#include <misc/printk.h>
#include "msg_manager.h"
#include "soc_reset.h"

#define SYS_LOG_DOMAIN "audio"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

static struct audioin_handle audioin_handle_entity;
static struct audioin_handle *audioin;
static void audio_pcm_buf_cbk(uint32_t pending)
{
	if (!audioin || !audioin->running)
		return;

	if ((pending & PCMBUF_IP_HF) || (pending & PCMBUF_IP_FU)) {
		if (pending & PCMBUF_IP_HF) {
			struct net_buf *buf = NULL;
			buf = net_buf_alloc(audioin->pcm_pool, K_NO_WAIT);
			if (buf == NULL) {
				audioin->completed_net_buf = NULL;
			} else {
				audio_in_pcmbuf_transfer_config(audioin->dev, (u16_t *)buf->data, buf->size);
				audioin->completed_net_buf = audioin->working_net_buf;
				audioin->working_net_buf = buf;
			}
		} else {
			if (audioin->completed_net_buf == NULL) {
				SYS_LOG_WRN("drop buffer");
			} else {
				net_buf_add(audioin->completed_net_buf, audioin->completed_net_buf->size);
				struct app_msg  msg = {0};
				msg.type = MSG_AUDIO_INPUT;
				msg.ptr = (void *)audioin->completed_net_buf;
				
				if (send_msg(&msg, K_NO_WAIT) == false) {
					/* free buf when send msg failed */
					SYS_LOG_WRN("free buf when send msg failed");
					net_buf_unref(audioin->completed_net_buf);
				}
			}
		}
	}
}



int audioin_start_record(void *handle)
{
	struct audioin_handle *audioin = (struct audioin_handle *)handle;
	struct net_buf *buf = NULL;
	pcmbuf_setting_t pcmbuf_setting;
	adc_setting_t adc_setting;
	ain_setting_t ain_setting;

	memset(&adc_setting, 0x0, sizeof(adc_setting));
	adc_setting.sample_rate = audioin->sample_rate;

	ain_setting.ain_src = AIN_SOURCE_MIC;
	adc_setting.gain = ADC_GAIN_0DB;
	ain_setting.op_gain.mic_op_gain = MIC_OP_30DB;

	pcmbuf_setting.irq_cbk = audio_pcm_buf_cbk;
	pcmbuf_setting.hf_ie = true;
	pcmbuf_setting.fu_ie = true;

	buf = net_buf_alloc(audioin->pcm_pool, K_NO_WAIT);
	if (buf == NULL) {
		SYS_LOG_WRN("alloc buffer failed\n");
		return -1;
	}

	audio_in_pcmbuf_config(audioin->dev, &pcmbuf_setting,(u16_t *)buf->data, buf->size);
	audioin->working_net_buf = buf;
	audioin->completed_net_buf = NULL;

	audio_in_start(audioin->dev, &ain_setting, &adc_setting);

	audioin->pcm_off = 0;
	audioin->running = true;
	audioin->rofs = 0;

	return 0;
}

int audioin_stop_record(void *handle)
{
	struct audioin_handle *audioin = (struct audioin_handle *)handle;

	audioin->running = false;

	audio_in_stop(audioin->dev);

	if (audioin && audioin->pcm_pool) {
		if (audioin->working_net_buf) {
			net_buf_unref(audioin->working_net_buf);
			audioin->working_net_buf = NULL;
		}
		
		if (audioin->completed_net_buf) {
			net_buf_unref(audioin->completed_net_buf);
			audioin->completed_net_buf = NULL;
		}

	}

	audioin->rofs = 0;
	audioin->pcm_off = 0;

	return 0;
}

void *audioin_device_open(u8_t sample_rate)
{
	struct device *audio_dev;

	audio_dev = device_get_binding(CONFIG_AUDIO_IN_ACTS_NAME);

	if (!audio_dev) {
		SYS_LOG_ERR("fail get audio_dev");
		return NULL;
	}

	audioin = &audioin_handle_entity;

	audioin->dev = audio_dev;

	audioin->sample_rate = sample_rate;

	return audioin;
}
void audioin_device_close(void *handle)
{
	unsigned int key;

	key = irq_lock();

	if (audioin->pcm_pool)
		audioin->pcm_pool = NULL;


	audioin = NULL;

	irq_unlock(key);
}
