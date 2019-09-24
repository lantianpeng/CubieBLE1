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

#ifndef _SYSTEM_APP_SM_H_
#define _SYSTEM_APP_SM_H_

/*Rmc State machine states */
enum {
	RMC_STATES_IDLE,
		RMC_STATES_ADVERTISING,
		RMC_STATES_CONNECTED,
		RMC_STATES_NUM,
};

/* State machine actions */
enum {
	RMC_SM_ACT_NONE,
	RMC_SM_ACT_ADV,
	RMC_SM_ACT_DIRECT_ADV,
	RMC_SM_ACT_STOP_ADV,
	RMC_SM_ACT_DISCONNECT_CONN,
	RMC_SM_ACT_SENDKEY,
	RMC_SM_ACT_CONNECTED,
};

/* RMC event handler messages for state machine */
enum {
	/* messages from input */
	RMC_MSG_INPUT_KEY,
	/* messages from BLE */
	RMC_MSG_BLE_CONNECTED,
	RMC_MSG_BLE_SEC_CONNECTED,
	RMC_MSG_BLE_INPUT_ENABLED,
	RMC_MSG_BLE_USER_DISCONNECTED,
	RMC_MSG_BLE_NON_USER_DISCONNECTED,

	/* messages from appTimer */
	RMC_MSG_TIMER_ADV_TIMEOUT,
	RMC_MSG_TIMER_DIRECT_ADV_TIMEOUT,
	RMC_MSG_TIMER_NO_ACT_TIMEOUT,

	/* message from others */
	RMC_MSG_RECONN_FAIL,
	RMC_MSG_POWER_UP,
	RMC_MSG_CLEAR_PIAR_KEY,
	RMC_MSG_LOW_POWER,
	RMC_NUM_MSGS
};

/* Connection control block */
struct rmc_cb_t {
	u8_t  rmc_status;
};

extern struct rmc_cb_t g_rmc_cb;

/* Action function */
typedef void (*rmc_act_t)(u8_t event, void *msg);

/**
 * @brief state machine of application initialization
 *
 */

void rmc_sm_init(void);

/**
 * @brief change state of application from one state to another state
 *
 * @param event which will trigger a state change.
 *
 */

void rmc_sm_execute(u8_t evt);

/**
 * @brief check whether the current state is activing
 *
 *
 * @return true when state is not RMC_STATES_CONNECTED.
 * @return false when state is RMC_STATES_CONNECTED.
 */

bool rmc_sm_state_is_activing(void);

/**
 * @brief Take no action 
 *
 * @param event which will trigger a state change.
 * @param msg from state machine.
 *
 */

void rmc_sm_act_none(u8_t event, void *msg);

/**
 * @brief Start broadcasting
 *
 * @param event which will trigger a state change.
 * @param msg from state machine.
 *
 */

void rmc_sm_act_start_adv(u8_t event, void *msg);

/**
 * @brief Start direct broadcasting
 *
 * @param event which will trigger a state change.
 * @param msg from state machine.
 *
 */

void rmc_sm_act_start_direct_adv(u8_t event, void *msg);

/**
 * @brief Stop broadcasting
 *
 * @param event which will trigger a state change.
 * @param msg from state machine.
 *
 */

void rmc_sm_act_stop_adv(u8_t event, void *msg);

/**
 * @brief disconnect the connection
 *
 * @param event which will trigger a state change.
 * @param msg from state machine.
 *
 */

void rmc_sm_act_disconnect_conn(u8_t event, void *msg);

/**
 * @brief send hid key through ble
 *
 * @param event which will trigger a state change.
 * @param msg from state machine.
 *
 */

void rmc_sm_act_send_key(u8_t event, void *msg);

void rmc_sm_act_connected(u8_t event, void *msg);


#endif

