/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <string.h>
#include <audio.h>

#include "audio_out_hal.h"
#include <net/buf.h>
#include <misc/printk.h>

static struct audio_out_handle audio_out_handle_entity;
static struct audio_out_handle * audio_out;

static os_work put_pcm_work;
extern void audio_out_pcmbuf_transfer_config(struct device *dev,uint16_t *pcm_buf, u32_t data_cnt);

static void audio_pcm_buf_cbk(uint32_t pending)
{
	if(!audio_out || !audio_out->running)
	{
		return;
	}
	if((pending & PCMBUF_IP_HF) || (pending & PCMBUF_IP_FU))
	{

		if((pending & PCMBUF_IP_HF))
		{
			audio_out_pcmbuf_transfer_config(audio_out->dev, (uint16_t *)(&audio_out->pcm_buf[(audio_out->pcm_off + audio_out->pcm_buf_len/2) % audio_out->pcm_buf_len]), audio_out->pcm_buf_len/2);
		}
		else
		{
			if (audio_out->pcm_off) {
					audio_out->pcm_off = 0;
			} else {
					audio_out->pcm_off = audio_out->pcm_buf_len/2;
			}
			os_work_submit_to_queue(&k_sys_work_q, &put_pcm_work);
		}
	}
}

extern void send_pcm_to_i2s(u8_t *p_out, u16_t p_out_len);

void put_pcm_data(os_work *work)
{
	if (audio_out && audio_out->pcm_pool) {
		int data_len = 0;
		u8_t *user_pcm_data = NULL;
		
		user_pcm_data = (u8_t *)&audio_out->pcm_buf[(audio_out->pcm_off + audio_out->pcm_buf_len/2) % audio_out->pcm_buf_len];
		
		data_len = audio_out->pcm_buf_len / 2;
		
		send_pcm_to_i2s(user_pcm_data, data_len);
	}
}

int audio_out_get_pcm_data(void *handle, char **pcm, int len)
{

	return 0;
}

extern void audio_out_enable(struct device *dev, ain_setting_t *ain_setting,
		     adc_setting_t *adc_setting);
extern void audio_out_pcmbuf_config(struct device *dev, pcmbuf_setting_t *setting, uint16_t *pcm_buf, u32_t data_cnt);
int audio_out_start_playback(void *handle)
{
	struct audio_out_handle * audio_out = (struct audio_out_handle *)handle;
	pcmbuf_setting_t pcmbuf_setting;
	adc_setting_t adc_setting;
	ain_setting_t ain_setting;

	memset(&adc_setting, 0x0, sizeof(adc_setting));
	adc_setting.sample_rate = audio_out->sample_rate;

	ain_setting.ain_src = AIN_SOURCE_MIC;
	adc_setting.gain = ADC_GAIN_6DB;
	ain_setting.op_gain.mic_op_gain = MIC_OP_21DB_1;

	
	pcmbuf_setting.irq_cbk = audio_pcm_buf_cbk;
	pcmbuf_setting.hf_ie = true;
	pcmbuf_setting.fu_ie = true;
	
	audio_out_pcmbuf_config(audio_out->dev, &pcmbuf_setting, (uint16_t *)audio_out->pcm_buf,
						audio_out->pcm_buf_len/2);
  //printk("audio_out->pcm_buf_len is %d\n",audio_out->pcm_buf_len);
	audio_out_enable(audio_out->dev, &ain_setting, &adc_setting);
	
	audio_out->pcm_off = 0;
	audio_out->running = true;
	audio_out->rofs = 0;

	return 0;
}
#define	RESET_ID_AUDIO			16
extern void acts_reset_peripheral(int reset_id);
int audio_out_stop_playback(void *handle)
{
	struct audio_out_handle * audio_out = (struct audio_out_handle *)handle;

	audio_out->running = false;
	
	audio_out_disable(audio_out->dev);
	acts_reset_peripheral(RESET_ID_AUDIO);

	if(audio_out && audio_out->pcm_queue) {
		/* free all buffers of pcm_buffer_pool*/
		struct net_buf *buf = NULL;
		do {
			buf = net_buf_get(audio_out->pcm_queue, K_NO_WAIT);
			if (buf != NULL) {
				net_buf_unref(buf);
				printk("free overflow buffer\n");
			}
		}while (buf);
	}

	audio_out->rofs = 0;
	audio_out->pcm_off = 0;

	return 0;
}

void* audio_out_device_open(uint8_t sample_rate)
{
	struct device *audio_dev;
	
	audio_dev = device_get_binding(CONFIG_AUDIO_OUT_ACTS_NAME);
	
	if (!audio_dev) {
		return NULL;
	} 
	
	audio_out = &audio_out_handle_entity;
	
	audio_out->dev = audio_dev;
	
	audio_out->sample_rate = sample_rate;

	os_work_init(&put_pcm_work, put_pcm_data);
	printk("audio_out_device_open successful\n");
	return audio_out;
}
void audio_out_device_close(void *handle)
{
	unsigned int key;
	key = irq_lock();

	if(audio_out->pcm_pool) {
		audio_out->pcm_pool = NULL;
	}
	if(audio_out->pcm_queue) {
		audio_out->pcm_queue = NULL;
	}

	audio_out = NULL;
	
	irq_unlock(key);
	return ;
}
