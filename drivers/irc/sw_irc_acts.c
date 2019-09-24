/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>
#include <irc.h>
#include "sw_irc_acts.h"
#include <gpio.h>
#include <pwm.h>
#include <misc/printk.h>
#include <string.h>

#define SYS_LOG_DOMAIN "IRKEY"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_INPUT_DEV_LEVEL
#include <logging/sys_log.h>

#ifdef CONFIG_IRC_SW_RX

#define CONFIG_USE_TIMER3_FOR_DBEUG 0

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct sw_acts_irc_config *)(dev)->config->config_info)
		
#define     T2_CTL                                                            (TIMER_REG_BASE+0X0080)
#define     T2_VAL                                                            (TIMER_REG_BASE+0X0084)
#define     T2_CNT                                                            (TIMER_REG_BASE+0X0088)
#define     T2_CAP                                                            (TIMER_REG_BASE+0X008C)
#define     T3_CTL                                                            (TIMER_REG_BASE+0X00C0)
#define     T3_VAL                                                            (TIMER_REG_BASE+0X00C4)
#define     T3_CNT                                                            (TIMER_REG_BASE+0X00C8)
#define     T3_CAP                                                            (TIMER_REG_BASE+0X00CC)

#define     CMU_TIMER2CLK                                                     (CMU_DIGITAL_BASE+0X0044)
#define     CMU_TIMER3CLK                                                     (CMU_DIGITAL_BASE+0X0048)
	
#if CONFIG_USE_TIMER3_FOR_DBEUG
#define COMMON_TIMER_IRQ_ID IRQ_ID_TIMER1
#define COMMON_TIMER_CLOCK_ID CLOCK_ID_TIMER1
#define COMMON_TIMER_CTL T1_CTL
#define COMMON_TIMER_VAL T1_VAL
#define COMMON_TIMER_CNT T1_CNT
#define COMMON_TIMER_CMU_CLK  CMU_TIMER1CLK
#define     RTC_DEBUGSEL                                                      (RTC_REG_BASE+0X0080)
#define     DEBUGSEL                                                          (IO_MFP_BASE+0X200)
#define     DEBUGIE                                                           (IO_MFP_BASE+0X204)
#define     DEBUGOE                                                           (IO_MFP_BASE+0X208)
#else
#define COMMON_TIMER_IRQ_ID IRQ_ID_ID_TIMER3
#define COMMON_TIMER_CLOCK_ID CLOCK_ID_TIMER3
#define COMMON_TIMER_CTL T3_CTL
#define COMMON_TIMER_VAL T3_VAL
#define COMMON_TIMER_CNT T3_CNT
#define COMMON_TIMER_CMU_CLK  CMU_TIMER3CLK
#endif

#define CAP_TIMER_IRQ_ID  IRQ_ID_ID_TIMER2
#define CAP_TIMER_CLOCK_ID CLOCK_ID_TIMER2
#define CAP_TIMER_CMU_CLK  CMU_TIMER2CLK
#define CAP_TIMER_CTL T2_CTL
#define CAP_TIMER_VAL T2_VAL
	

#define		DISABLE						0
#define		ENABLE						1

#define		COUNTER_1_US							(32)
#define		TIMER_VALUE_SET						(0xffffff00)
	
#define  	TX_CARRIER_FREQ_DEFAULT  			(38000)
#define 	TX_REPEAT_GAP_TIME_DEFAULT    (108 * 1000)
	
u16_t pulse_width_tab[MAX_PULSE_LEN];
u16_t pulse_width_tab_len;

/* us delay */
__ramfunc void us_delay(u32_t num)
{
	u32_t counter = COUNTER_1_US * num;
	u32_t counter_start = 0;
	u32_t counter_end = 0;
	u32_t cnt_check;

	counter_start = sys_read32(COMMON_TIMER_CNT);
	cnt_check = counter_start;
	if (counter_start >= counter) {
		counter_end = counter_start - counter;
		while (1) {
			volatile u32_t t_cnt = sys_read32(COMMON_TIMER_CNT);
			if (t_cnt <= counter_end)
				break;												
			else if (t_cnt > counter_start)
				break;
			/* maybe timer is stop for some reason */
			if (cnt_check == t_cnt)
				break;
			cnt_check = t_cnt;
		}
	} else {
		counter_end = TIMER_VALUE_SET - (counter - counter_start);
		while (1) {
			volatile u32_t t_cnt = sys_read32(COMMON_TIMER_CNT);
			if ((t_cnt > counter_start) && (t_cnt <= counter_end))
				break;
			/* maybe timer is stop for some reason */
			if (cnt_check == t_cnt)
				break;
			cnt_check = t_cnt;
		}
	}
}

u32_t cap_lvl_read(void)
{
	return (sys_read32(CAP_TIMER_CTL)>>12)&0x1;
}

u32_t timer_read(void)
{
	return sys_read32(COMMON_TIMER_CNT);
}

void timer_cfg(u8_t enable)
{
	if (enable) {
		/* enable timer1 clock */
		acts_clock_peripheral_enable(COMMON_TIMER_CLOCK_ID);

		sys_write32(0x0, COMMON_TIMER_CMU_CLK);

		sys_write32(0x1, COMMON_TIMER_CTL);
		sys_write32(TIMER_VALUE_SET, COMMON_TIMER_VAL);
		sys_write32(0x24, COMMON_TIMER_CTL);
	} else {
		sys_write32(0x1, COMMON_TIMER_CTL);
		sys_write32(0x0, COMMON_TIMER_VAL);
		
		/* diable timer clock */
		acts_clock_peripheral_disable(COMMON_TIMER_CLOCK_ID);
	}
}

__ramfunc void start_common_timer(u32_t delay_time)
{
	/* enable timer2 clock */
	irq_enable(COMMON_TIMER_IRQ_ID);
	acts_clock_peripheral_enable(COMMON_TIMER_CLOCK_ID);
	sys_write32(0x0, COMMON_TIMER_CMU_CLK);

	sys_write32(0x1, COMMON_TIMER_CTL);
	sys_write32(delay_time * COUNTER_1_US, COMMON_TIMER_VAL);       /* 108 ms */
	sys_write32(0x22, COMMON_TIMER_CTL);		/* timer */
}

__ramfunc void stop_common_timer(void)
{
	sys_write32(0x1, COMMON_TIMER_CTL);
	sys_write32(0x0, COMMON_TIMER_VAL);
	
	/* diable timer clock */
	acts_clock_peripheral_disable(COMMON_TIMER_CLOCK_ID);
	irq_disable(COMMON_TIMER_IRQ_ID);
}

void capture_cfg(u8_t enable)
{
	if (enable) {
		/* enable timer2 clock */
		acts_clock_peripheral_enable(CAP_TIMER_CLOCK_ID);

		sys_write32(0x0, CAP_TIMER_CMU_CLK);

		sys_write32(0x1, CAP_TIMER_CTL);
		sys_write32(0x24A2, CAP_TIMER_CTL);		/* signal from IRC RX analog in part */
	} else {
		sys_write32(0x1, CAP_TIMER_CTL);
		sys_write32(0x0, CAP_TIMER_VAL);
		
		/* diable timer clock */
		acts_clock_peripheral_disable(CAP_TIMER_CLOCK_ID);
	}
}

void start_irc_tx_timer(u32_t delay_time)
{
	/* enable timer2 clock */
	acts_clock_peripheral_enable(CAP_TIMER_CLOCK_ID);
	sys_write32(0x0, CAP_TIMER_CMU_CLK);

	sys_write32(0x1, CAP_TIMER_CTL);
	sys_write32(delay_time * COUNTER_1_US, CAP_TIMER_VAL);       /* 108 ms */
	sys_write32(0x22, CAP_TIMER_CTL);		/* timer */
}

void stop_irc_tx_timer(void)
{
	sys_write32(0x1, CAP_TIMER_CTL);
	sys_write32(0x0, CAP_TIMER_VAL);
	
	/* diable timer clock */
	acts_clock_peripheral_disable(CAP_TIMER_CLOCK_ID);
}

__ramfunc static inline u32_t get_time_count(u32_t counter_start, u32_t counter_end)
{
	u32_t tmp_cnt;
	if (counter_start >= counter_end) {
		tmp_cnt = counter_start - counter_end;
	} else {
		tmp_cnt = TIMER_VALUE_SET - counter_end + counter_start;
	}
	return tmp_cnt;
}

/* params
seq_to_ms : key check timeout check,unit ms
check_period_us : key check freq,unit us
check_wait : key checking wait,unit us
*/
__ramfunc bool cap_irc(int seq_to_ms,int check_period_us,int check_to_us)
{
	volatile u32_t cnt_to,cnt_wth=0;
	volatile u32_t carrier_timer_start, space_timer_start;
	volatile u32_t seq_to_timer_start, seq_to_timer_end, seq_to_counter, check_counter;
	u32_t bit_wth;
	
	u16_t *p_pluse_wth = (u16_t*)pulse_width_tab;
	u32_t tmp_pluse_cnt;
	pulse_width_tab_len = 0;
	
	seq_to_counter = seq_to_ms * 1000 * COUNTER_1_US;
	check_to_us = check_to_us / check_period_us;

	bit_wth = 0;
	while (1) {
		volatile u32_t cnt0, cnt1;
		
		/* mark modulation start timer,non-modulated freq */
		if(bit_wth !=0){
			carrier_timer_start = timer_read();
			tmp_pluse_cnt = get_time_count(space_timer_start, carrier_timer_start);
			*p_pluse_wth++ = tmp_pluse_cnt / COUNTER_1_US;
			bit_wth++;
		} else {
			carrier_timer_start = timer_read();
		}

		cnt_to = 0;
		cnt_wth = 0;
		cnt0 = cap_lvl_read();
		seq_to_timer_start = timer_read();
		check_counter = seq_to_timer_start;
		while(1){
			cnt1 = cap_lvl_read();
			if(cnt1 == cnt0){
				cnt_to++;
				if (cnt_to == 1) {
					space_timer_start = timer_read();
				}
			}
			else{
				cnt_to=0;
				cnt_wth++;
			}
			cnt0 = cnt1;
			/* timeout occur */
			if (cnt_to >= check_to_us) {
				break;
			}

			us_delay(check_period_us);
			/* whole seq timeout */
			seq_to_timer_end = timer_read();
			/* maybe it is not possible */
			if (check_counter == seq_to_timer_end)
				break;
			check_counter = seq_to_timer_end;
			if (get_time_count(seq_to_timer_start, seq_to_timer_end) >= seq_to_counter)
				return false;
		}

		cnt0 = cnt1;
		/* calculate the mod_freq and pluse width */
		tmp_pluse_cnt = get_time_count(carrier_timer_start, space_timer_start);
		*p_pluse_wth++ = tmp_pluse_cnt / COUNTER_1_US;
		bit_wth++;
		
		/* detect level interval */	
		seq_to_timer_start = timer_read();
		check_counter = seq_to_timer_start;
		cnt0 = cap_lvl_read();
		while (cap_lvl_read() == cnt0) {
			us_delay(check_period_us);
			/* whole seq timeout */
			seq_to_timer_end = timer_read();
			/* maybe it is not possible */
			if (check_counter == seq_to_timer_end)
				break;
			check_counter = seq_to_timer_end;
			if (get_time_count(seq_to_timer_start, seq_to_timer_end) >= seq_to_counter) {
				if (bit_wth) {
					pulse_width_tab_len = bit_wth;
					/*compensate 50 us for costing from irq to start read carrier_timer_start*/
					pulse_width_tab[0] = pulse_width_tab[0] + 50;
					return true;
				} else {
					return false;
				}
			}
		}
		
		if(bit_wth > MAX_PULSE_LEN) 
			return false;

	}
}


void cap_analysis(struct sw_acts_irc_data *drv_data)
{
	struct ir_input_value val;

	/* TODO */
	
	if (drv_data->notify) {
		val.type = IR_FORMAT_PULSE_TAB;
		val.pluse_tab = pulse_width_tab;
		val.pluse_tab_len = pulse_width_tab_len;
		drv_data->notify(NULL, (struct input_value*)&val);
	}	
}

__ramfunc void pwm_output_carrier(struct device *dev, u32_t freq, u32_t time)
{
	u32_t temp;
	temp = 32000000/freq;

	/* mfp switch from gpio to pwm func */
	acts_pinmux_set(CONFIG_IRC_TX_PIN, 0x2 | (0x3 << 12)); 
	pwm_pin_set_cycles(dev, 4, temp, temp/2);
	
	start_common_timer(time);
}

__ramfunc void pwm_output_lowlevel(struct device *dev, u32_t time)
{
	/* mfp switch from pwm to gpio func */
	acts_pinmux_set(CONFIG_IRC_TX_PIN, 0x1 << 6);
	gpio_pin_write(dev, CONFIG_IRC_TX_PIN, 0);
	
	start_common_timer(time);
}

__ramfunc void common_timer_isr(struct device *dev)
{
	struct sw_acts_irc_data *drv_data = dev->driver_data;
	
	drv_data->cur_read_pulse_tab_index++;
	
	if (drv_data->cur_read_pulse_tab_index % 2 == 0) {
		pwm_output_carrier(drv_data->pwm_dev, TX_CARRIER_FREQ_DEFAULT, drv_data->p_pulse_tab[drv_data->cur_read_pulse_tab_index]);
	} else {
		pwm_output_lowlevel(drv_data->gpio_dev, drv_data->p_pulse_tab[drv_data->cur_read_pulse_tab_index]);
	}
	
	/* clear irq pending */
	sys_write32(0x1 | sys_read32(COMMON_TIMER_CTL), COMMON_TIMER_CTL);
	
	if (drv_data->cur_read_pulse_tab_index >= drv_data->pulse_tab_len)
	{
		/* stop pwm */
		pwm_pin_set_cycles(drv_data->pwm_dev, 4, 0, 0);
		
		/* stop common timer */
		stop_common_timer();
		
		/* start repeat code timer */
		start_irc_tx_timer(TX_REPEAT_GAP_TIME_DEFAULT);
		printk("start repeat timer\n");
	}
}

void sw_irc_output_ircode(struct device *dev, u16_t *p_pulse_tab, u16_t pulse_count)
{
	struct sw_acts_irc_data *drv_data = dev->driver_data;
	
	if (!drv_data->gpio_dev) {
		drv_data->gpio_dev = device_get_binding(CONFIG_GPIO_ACTS_DRV_NAME);
		if (!drv_data->gpio_dev) {
			printk("cannot found device \'%s\'\n",
						CONFIG_GPIO_ACTS_DRV_NAME);
			return;
		}
	}
	if (!drv_data->pwm_dev) {
		drv_data->pwm_dev = device_get_binding(CONFIG_PWM_ACTS_DEV_NAME);
		if (!drv_data->pwm_dev) {
			printk("cannot found device \'%s\'\n",
						CONFIG_PWM_ACTS_DEV_NAME);
			return;
		}
	}
	
	/* TODO add a interface to pwm drivers */
	sys_write32(0, CMU_PWM3CLK);
	
	/* set IR_TX_IO_CTL as writer */
	gpio_pin_configure(drv_data->gpio_dev, CONFIG_IRC_TX_PIN, GPIO_DIR_OUT);
	gpio_pin_write(drv_data->gpio_dev, CONFIG_IRC_TX_PIN, 0);
	
	drv_data->cur_read_pulse_tab_index = 0;
	drv_data->p_pulse_tab = p_pulse_tab;
	drv_data->pulse_tab_len = pulse_count;
	

	pwm_output_carrier(drv_data->pwm_dev, TX_CARRIER_FREQ_DEFAULT, p_pulse_tab[drv_data->cur_read_pulse_tab_index]);

}


int sw_irc_acts_output_key(struct device *dev, struct input_value *ir_val)
{
	unsigned int key;
	struct sw_acts_irc_data *drv_data = dev->driver_data;
	struct ir_input_value *val = (struct ir_input_value *)ir_val;
	
	printk("val->value %d val->code %d val->pluse_tab_len %d\n", val->value, val->code, val->pluse_tab_len);
	key = irq_lock();
	
	/* diable learn mode first when send */
	if (drv_data->learn_mode == 1) {
		((struct input_dev_driver_api *)dev->driver_api)->disable(dev);
	}
	
	drv_data->key_state = val->value;
	drv_data->key_code = val->code;
	
	if ((drv_data->key_state == IR_KEY_DOWN) && (drv_data->busy == 0)) {	
		if (val->pluse_tab_len) {
			pulse_width_tab_len = val->pluse_tab_len;
			memcpy(pulse_width_tab, val->pluse_tab, sizeof(u16_t) * val->pluse_tab_len);
		} else {
			/*TODO */
			goto __func_end;
		}
		
		device_busy_set(dev);
	
		drv_data->old_mfp_ctl = sys_read32(IR_TX_IO_CTL);
		
		sw_irc_output_ircode(dev, pulse_width_tab, pulse_width_tab_len);
		
		drv_data->busy = 1;
		drv_data->pre_key_code = drv_data->key_code;
		printk("output first code\n");
	}
__func_end:
	irq_unlock(key);

	return 0;
}

__ramfunc void sw_irc_acts_isr(struct device *dev)
{
	struct sw_acts_irc_data *drv_data = dev->driver_data;
	
	if (drv_data->learn_mode) { /* rx learn */
		unsigned int key;
		key = irq_lock();

		if (cap_irc(20,5,150)) {
			cap_analysis(drv_data);
		}
		u32_t val = sys_read32(CAP_TIMER_CTL);
		sys_write32(val | (1 << 8), CAP_TIMER_CTL);
		sys_write32(0x1, CAP_TIMER_CTL);
		sys_write32(0x24A2, CAP_TIMER_CTL);
		irq_unlock(key);
		printk("enable irq 0x%x\n",	sys_read32(CAP_TIMER_CTL));
		
	} else { /* Tx output */
		if (drv_data->key_state == IR_KEY_DOWN) {
			sw_irc_output_ircode(dev, pulse_width_tab, pulse_width_tab_len);
			
			start_irc_tx_timer(TX_REPEAT_GAP_TIME_DEFAULT);
			printk("output repeat code\n");
			drv_data->busy = 1;
		} else {
			/* TODO */
			drv_data->busy = 0;
			sys_write32(drv_data->old_mfp_ctl, IR_TX_IO_CTL);  /* tmp do it */
			device_busy_clear(dev);
			printk("release key\n");
		}

		drv_data->pre_key_code = drv_data->key_code;
		/* clear irq pending */
		sys_write32(0x1 | sys_read32(CAP_TIMER_CTL), CAP_TIMER_CTL);
	}
}

void sw_irc_input_acts_register_notify(struct device *dev, input_notify_t notify)
{
	struct sw_acts_irc_data *drv_data = dev->driver_data;

	SYS_LOG_DBG("register notify 0x%x", (u32_t)notify);

	drv_data->notify = notify;
}

void sw_irc_input_acts_unregister_notify(struct device *dev, input_notify_t notify)
{
	struct sw_acts_irc_data *drv_data = dev->driver_data;

	SYS_LOG_DBG("unregister notify 0x%x", (u32_t)notify);

	drv_data->notify = NULL;
}

void sw_irc_input_acts_enable(struct device *dev)
{
	const struct sw_acts_irc_config *cfg = DEV_CFG(dev);
	struct sw_acts_irc_data *drv_data = dev->driver_data;

	SYS_LOG_DBG("enable irc input");
	
	unsigned int key;
	key = irq_lock();
	
#if CONFIG_USE_TIMER3_FOR_DBEUG
	sys_write32(0x06, DEBUGSEL);
	sys_write32(1<<18,DEBUGOE);
         
	acts_clock_peripheral_enable(CLOCK_ID_TIMER3);
	sys_write32(0, RTC_DEBUGSEL);
	sys_write32((1<<13)|(0x2<<9)|(1<<5)|(1<<2),T3_CTL);
#endif
	
	if (drv_data->busy) {
			drv_data->busy = 0;
			sys_write32(drv_data->old_mfp_ctl, IR_TX_IO_CTL);  /* tmp do it */
			device_busy_clear(dev);
			/* stop pwm */
			pwm_pin_set_cycles(drv_data->pwm_dev, 4, 0, 0);
			stop_common_timer();
			stop_irc_tx_timer();
			drv_data->cur_read_pulse_tab_index = 0;
			printk("stop tx\n");
	}

	/* reset irc controller */
	acts_reset_peripheral(cfg->reset_id);
	/* enable IRC clock */
	acts_clock_peripheral_enable(cfg->clock_id);
	/* enable IRC RX clock */
	acts_clock_peripheral_enable(cfg->rx_clock_id);
	
	sys_write32(0x91, IR_RX_IO_CTL);

	sys_write32(sys_read32(IRC_ANA_CTL) | (0x1<<1), IRC_ANA_CTL);
	sys_write32(sys_read32(IRC_ANA_CTL) | (0x1<<2), IRC_ANA_CTL);
	k_busy_wait(1000);
	sys_write32(sys_read32(IRC_ANA_CTL) & ~(0x1<<2), IRC_ANA_CTL);
	sys_write32(sys_read32(IRC_ANA_CTL) | (0x1<<0), IRC_ANA_CTL);
	sys_write32(sys_read32(IRC_ANA_CTL) | (0x2aa0), IRC_ANA_CTL);
	
	timer_cfg(ENABLE);
	capture_cfg(ENABLE);
	
	device_busy_set(dev);
	
	drv_data->learn_mode = 1;
	
	irq_unlock(key);
}

void sw_irc_input_acts_disable(struct device *dev)
{
	const struct sw_acts_irc_config *cfg = DEV_CFG(dev);
	struct sw_acts_irc_data *drv_data = dev->driver_data;

	SYS_LOG_DBG("disable irc input");
	unsigned int key;
	key = irq_lock();

	acts_clock_peripheral_disable(cfg->rx_clock_id);
	acts_clock_peripheral_disable(cfg->clock_id);
	acts_reset_peripheral(cfg->reset_id);

	sys_write32(0x0, IR_RX_IO_CTL);
	
	capture_cfg(DISABLE);
	
	timer_cfg(DISABLE);
	
	device_busy_clear(dev);
	
	drv_data->learn_mode = 0;
	irq_unlock(key);
}

const struct irc_driver_api sw_irc_acts_drv_api_funcs = {
	.enable = sw_irc_input_acts_enable,
	.disable = sw_irc_input_acts_disable,
	.register_notify = sw_irc_input_acts_register_notify,
	.unregister_notify = sw_irc_input_acts_unregister_notify,
	.irc_output = sw_irc_acts_output_key,
};

int sw_irc_acts_init(struct device *dev);

struct sw_acts_irc_data  sw_irc_acts_drv_data;

const struct sw_acts_irc_config sw_irc_acts_dev_cfg_info = {
	.base = IRC_REG_BASE,
	.clock_id = CLOCK_ID_IRC_CLK,
	.reset_id = RESET_ID_IRC,
	.rx_clock_id = CLOCK_ID_IRC_RXCLK,
};


DEVICE_AND_API_INIT(sw_irc_acts, CONFIG_SW_IRC_ACTS_DEV_NAME,
		    sw_irc_acts_init,
		    &sw_irc_acts_drv_data, &sw_irc_acts_dev_cfg_info,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &sw_irc_acts_drv_api_funcs);

int sw_irc_acts_init(struct device *dev)
{
	IRQ_CONNECT(CAP_TIMER_IRQ_ID, 0,
		    sw_irc_acts_isr, DEVICE_GET(sw_irc_acts), 0);
	irq_enable(CAP_TIMER_IRQ_ID);

	IRQ_CONNECT(COMMON_TIMER_IRQ_ID, 0,
		    common_timer_isr, DEVICE_GET(sw_irc_acts), 0);
	irq_enable(COMMON_TIMER_IRQ_ID);

	return 0;
}
#endif
