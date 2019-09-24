/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SYSTEM_APP_TIMER_H_
#define _SYSTEM_APP_TIMER_H_

enum {
	ID_ADV_TIMEOUT,
	ID_DIRECT_ADV_TIMEOUT,
	ID_NO_ACT_TIMEOUT,
	ID_ADC_BATTERY_TIMEOUT,
	ID_PAIR_COMB_KEY_TIMEOUT,
	ID_HCI_MODE_COMB_KEY_TIMEOUT,
	ID_RMC_SEC_TIMEOUT,
#ifdef CONFIG_IRC_SW_RX
	ID_IR_LEARN_KEY_TIMEOUT,
	ID_IR_LEARN_COMB_KEY_MASK_TIMEOUT,
#endif
	ID_NUM_TIMEOUTS,
};

#define CONFIG_RMC_ADV_TIMEOUT            30
#define CONFIG_RMC_DIRECT_ADV_TIMEOUT     2
#define CONFIG_RMC_NO_ACT_TIMEOUT         (5*60)
#define CONFIG_RMC_ADC_BATTERY_TIMEOUT    (30)
#define CONFIG_RMC_PAIR_COMB_KEY_TIMEOUT  2
#define CONFIG_RMC_HCI_MODE_COMB_KEY_TIMEOUT  5
#define CONFIG_RMC_SEC_TIMEOUT   6
#ifdef CONFIG_IRC_SW_RX
#define CONFIG_RMC_LEARN_KEY_TIMEOUT 3
#define cONFIG_IR_LEARN_COMB_KEY_MASK_TIMEOUT 500
#endif
/**
 * @brief stop one of application timer
 *
 * @param application timer id.
 *
 */

void rmc_timer_stop(int8_t timer_id);

/**
 * @brief start one of application timer
 *
 * @param application timer id.
 *
 */

void rmc_timer_start(int8_t timer_id);

/**
 * @brief application timer initialization
 *
 *
 * @return 0  init success
 * @return other init failed
 */

int8_t rmc_timer_init(void);

/**
 * @brief event of application timer handler
 *
 * @param timer event.
 */

void system_app_timer_event_handle(u32_t timer_event);

#endif
