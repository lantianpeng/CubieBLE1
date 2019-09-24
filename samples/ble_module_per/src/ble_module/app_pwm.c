/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <gpio.h>
#include <pwm.h>
#include <soc.h>
#include <misc/printk.h>
#include "app_pwm.h"

/* pwm base clk source is 1Mhz */
static const u32_t pwm_duty_max_tbl[20] = {
20000, /* 50   hz */
10000, /* 100  hz */
5000,  /* 200  hz */
2500,  /* 400  hz */
2000,  /* 500  hz */
1250,  /* 800  hz */
1000,  /* 1		Khz */
500,   /* 2		Khz */
250,   /* 4		Khz */ 
200,   /* 5		Khz */
125,   /* 8		Khz */
100,   /* 10	Khz */
50,    /* 20	Khz */
40,    /* 25  Khz */
25,    /* 40  Khz */
20,    /* 50  Khz */
10,    /* 100 Khz */
8,     /* 125 Khz */
5,     /* 200 Khz */
4,     /* 250 Khz */
};

static struct device *pwm_dev;

void pwm_cmd_handle(u8_t *cmd_buf)
{
	u8_t channel;
	u32_t duty_max_0, duty_0, duty_max_1, duty_1;
	
	/* check cmd paramter */
	if ((pwm_dev == NULL) ||
			(cmd_buf[0] != 0x97) ||
			(cmd_buf[1] > 3) ||
			(cmd_buf[2] > 20) ||
			(cmd_buf[3] > 100) ||
			(cmd_buf[4] > 20) ||
			(cmd_buf[5] > 100)
			) return ;
	
	if (cmd_buf[2] != 0)
		duty_max_0 = pwm_duty_max_tbl[cmd_buf[2] - 1];
	
	if (cmd_buf[3] != 0)
		duty_0 = duty_max_0 * cmd_buf[3] / 100;
	
	if (cmd_buf[4] != 0)
		duty_max_1 = pwm_duty_max_tbl[cmd_buf[4] - 1];
	
	if (cmd_buf[5] != 0)
		duty_1 = duty_max_1 * cmd_buf[5] / 100;
	
	//printk("set pwm %d, %d, %d, %d\n", cmd_buf[2], cmd_buf[3], cmd_buf[4], cmd_buf[5]);
	//printk("convert %d, %d, %d, %d\n",duty_max_0, duty_0, duty_max_1, duty_1);

	pwm_pol_set(pwm_dev, 1, 1);
	pwm_pol_set(pwm_dev, 3, 1);
	acts_pinmux_set(MODULE_PWM0_PIN, 0x2);
	acts_pinmux_set(MODULE_PWM1_PIN, 0x2);
	if (cmd_buf[0] == 0x97) {
		channel = cmd_buf[1]; 
		switch (channel) {
		case 0:  /* open pwm channel 1 & 3 */
			pwm_pin_set_cycles(pwm_dev, 1, duty_max_0, duty_0);
			pwm_pin_set_cycles(pwm_dev, 3, duty_max_1, duty_1);
			break;
		case 1:  /* close pwm channel 1 & 3 */
			pwm_pin_set_cycles(pwm_dev, 1, duty_max_0, 0);
			pwm_pin_set_cycles(pwm_dev, 3, duty_max_1, 0);
			break;
		case 2:  /* set pwm channel 1 */
			pwm_pin_set_cycles(pwm_dev, 1, duty_max_0, duty_0);
			break;
		case 3:  /* set pwm channel 3 */
			pwm_pin_set_cycles(pwm_dev, 3, duty_max_1, duty_1);
			break;
		}
	}
}

void app_pwm_init(void)
{
		pwm_dev = device_get_binding(CONFIG_PWM_ACTS_DEV_NAME);
		if (!pwm_dev) {
			printk("cannot found dev pwm");
			return;
		}
}
