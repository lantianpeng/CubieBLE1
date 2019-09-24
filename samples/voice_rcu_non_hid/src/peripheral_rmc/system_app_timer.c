/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <misc/printk.h>
#include <zephyr.h>

#include "msg_manager.h"
#include "system_app_timer.h"
#include "system_app_sm.h"
#include "system_app_batt.h"
#include "system_app_ble.h"
#include "bas.h"
#include "soc.h"

#define SYS_LOG_DOMAIN "timer"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

struct rmc_timer_t {
	struct k_delayed_work rmc_timer_work;
	s8_t timer_id;

};

static struct rmc_timer_t rmc_timer[ID_NUM_TIMEOUTS];

static u32_t rmcTimeoutValue[ID_NUM_TIMEOUTS] = {
	K_SECONDS(CONFIG_RMC_ADV_TIMEOUT),       /* 8   s   */
	K_SECONDS(CONFIG_RMC_DIRECT_ADV_TIMEOUT), /* 2 s   */
	K_SECONDS(CONFIG_RMC_NO_ACT_TIMEOUT),    /* 20 minutes  */
	K_SECONDS(CONFIG_RMC_ADC_BATTERY_TIMEOUT),       /* 10 minutes  */
	K_SECONDS(CONFIG_RMC_PAIR_COMB_KEY_TIMEOUT), /* 5 s */
	K_SECONDS(CONFIG_RMC_HCI_MODE_COMB_KEY_TIMEOUT), /* 3s */
};

void system_app_timer_event_handle(u32_t timer_event)
{
	switch (timer_event) {
	case ID_ADV_TIMEOUT:
		rmc_sm_execute(RMC_MSG_TIMER_ADV_TIMEOUT);
	break;
	case ID_DIRECT_ADV_TIMEOUT:
		rmc_sm_execute(RMC_MSG_TIMER_DIRECT_ADV_TIMEOUT);
	break;
	case ID_NO_ACT_TIMEOUT:
		rmc_sm_execute(RMC_MSG_TIMER_NO_ACT_TIMEOUT);
	break;
	case ID_ADC_BATTERY_TIMEOUT:
		/* notify battery again if state of sm is connected */
		if (!rmc_sm_state_is_activing()) {
			update_battry_value();
			bas_notify();
			rmc_timer_start(ID_ADC_BATTERY_TIMEOUT);
		}
	break;
	case ID_PAIR_COMB_KEY_TIMEOUT:
		rmc_clear_pair_info();
		rmc_sm_execute(RMC_MSG_CLEAR_PIAR_KEY);
	break;
	case ID_HCI_MODE_COMB_KEY_TIMEOUT:
		SYS_LOG_INF("enter into hci mode");
		sys_pm_reboot(REBOOT_TYPE_GOTO_DTM);
	break;
	default:
		SYS_LOG_ERR("error unkown timer event %d", timer_event);
	break;
	}
}

static void check_rmc_timer(struct k_work *work)
{
	struct app_msg  msg = {0};
	struct rmc_timer_t *timer = CONTAINER_OF(work, struct rmc_timer_t, rmc_timer_work);

	msg.type = MSG_APP_TIMER;
	msg.value = timer->timer_id;
	send_msg(&msg, K_MSEC(100));
}


void rmc_timer_stop(s8_t timer_id)
{
	k_delayed_work_cancel(&(rmc_timer[timer_id].rmc_timer_work));
}

void rmc_timer_start(s8_t timer_id)
{
	k_delayed_work_submit(&(rmc_timer[timer_id].rmc_timer_work), rmcTimeoutValue[timer_id]);
}

__init_once_text s8_t rmc_timer_init(void)
{
	int i = 0;

	for (i = 0; i < ID_NUM_TIMEOUTS; i++) {
		rmc_timer[i].timer_id = i;
		k_delayed_work_init(&(rmc_timer[i].rmc_timer_work), check_rmc_timer);
	}
	return 0;
}



