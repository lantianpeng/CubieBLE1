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

#include "system_app_sm.h"

#define SYS_LOG_DOMAIN "sm"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>


/* Column position of next state */
#define RMC_NEXT_STATE          0

/* Column position of action */
#define RMC_ACTION              1

/* Number of columns in the state machine state tables */
#define RMC_NUM_COLS            2

/*
 * RMC state machine state tables
 */
static const u8_t rmc_State_table[RMC_STATES_NUM][RMC_NUM_MSGS][RMC_NUM_COLS] = {
	/* RMC_STATES_IDLE state */
	{
	/* Event															Next state									Action */
	/* INPUT_KEY */												{RMC_STATES_ADVERTISING,    RMC_SM_ACT_DIRECT_ADV},
	/* RMC_MSG_BLE_CONNECTED */						{RMC_STATES_CONNECTED,      RMC_MSG_BLE_CONNECTED},
	/* RMC_MSG_BLE_SEC_CONNECTED */       {RMC_STATES_CONNECTED,      RMC_SM_ACT_SENDKEY},
	/* RMC_MSG_BLE_INPUT_ENABLED*/        {RMC_STATES_CONNECTED,      RMC_SM_ACT_NONE},
	/* RMC_MSG_BLE_USER_DISCONNECTED */		{RMC_STATES_IDLE,           RMC_SM_ACT_NONE},
	/*RMC_MSG_BLE_NON_USER_DISCONNECTED*/ {RMC_STATES_IDLE,           RMC_SM_ACT_DIRECT_ADV},
	/* RMC_MSG_TIMER_ADV_TIMEOUT*/				{RMC_STATES_IDLE,						RMC_SM_ACT_STOP_ADV},
	/* RMC_MSG_TIMER_DIRECT_ADV_TIMEOUT*/	{RMC_STATES_ADVERTISING,		RMC_SM_ACT_DIRECT_ADV},
	/* RMC_MSG_TIMER_NO_ACT_TIMEOUT */		{RMC_STATES_IDLE,           RMC_SM_ACT_NONE},
	/* RMC_MSG_TIMER_SEC_TIMEOUT */				{RMC_STATES_IDLE,           RMC_SM_ACT_NONE},
	/* RMC_MSG_RECONN_FAIL */							{RMC_STATES_IDLE,    				RMC_SM_ACT_NONE},
	/* RMC_MSG_POWER_UP */								{RMC_STATES_ADVERTISING,    RMC_SM_ACT_DIRECT_ADV},
	/* RMC_MSG_CLEAR_PIAR_KEY */  				{RMC_STATES_ADVERTISING,    RMC_SM_ACT_ADV},
	/* RMC_MSG_LOW_POWER     */   				{RMC_STATES_IDLE,    				RMC_SM_ACT_NONE},
	},
	/* RMC_STATES_ADVERTISING state */
	{
	/* Event															Next state									Action */
	/* INPUT_KEY */												{RMC_STATES_ADVERTISING,    RMC_SM_ACT_NONE},
	/* RMC_MSG_BLE_CONNECTED */						{RMC_STATES_CONNECTED,      RMC_SM_ACT_CONNECTED},
	/* RMC_MSG_BLE_SEC_CONNECTED */       {RMC_STATES_CONNECTED,      RMC_SM_ACT_SENDKEY},
	/* RMC_MSG_BLE_INPUT_ENABLED*/        {RMC_STATES_CONNECTED,      RMC_SM_ACT_NONE},
	/* RMC_MSG_BLE_USER_DISCONNECTED */		{RMC_STATES_IDLE,						RMC_SM_ACT_NONE},
	/*RMC_MSG_BLE_NON_USER_DISCONNECTED*/	{RMC_STATES_IDLE,						RMC_SM_ACT_NONE},
	/* RMC_MSG_TIMER_ADV_TIMEOUT*/				{RMC_STATES_IDLE,           RMC_SM_ACT_STOP_ADV},
	/* RMC_MSG_TIMER_DIRECT_ADV_TIMEOUT*/	{RMC_STATES_ADVERTISING,		RMC_SM_ACT_DIRECT_ADV},
	/* RMC_MSG_TIMER_NO_ACT_TIMEOUT */		{RMC_STATES_IDLE,						RMC_SM_ACT_STOP_ADV},
	/* RMC_MSG_TIMER_SEC_TIMEOUT */				{RMC_STATES_IDLE,           RMC_SM_ACT_NONE},
	/* RMC_MSG_RECONN_FAIL */							{RMC_STATES_IDLE,						RMC_SM_ACT_NONE},
	/* RMC_MSG_POWER_UP */								{RMC_STATES_ADVERTISING,    RMC_SM_ACT_DIRECT_ADV},
	/* RMC_MSG_CLEAR_PIAR_KEY */  				{RMC_STATES_ADVERTISING,    RMC_SM_ACT_ADV},
	/* RMC_MSG_LOW_POWER     */   				{RMC_STATES_IDLE,    				RMC_SM_ACT_STOP_ADV},
	},
	/* RMC_STATES_CONNECTED state */
	{
	/* Event															Next state									Action */
	/* INPUT_KEY */												{RMC_STATES_CONNECTED,      RMC_SM_ACT_NONE},
	/* RMC_MSG_BLE_CONNECTED */						{RMC_STATES_CONNECTED,      RMC_SM_ACT_NONE},
	/* RMC_MSG_BLE_SEC_CONNECTED */       {RMC_STATES_CONNECTED,      RMC_SM_ACT_SENDKEY},
	/* RMC_MSG_BLE_INPUT_ENABLED*/        {RMC_STATES_CONNECTED,      RMC_SM_ACT_NONE},
  /* RMC_MSG_BLE_USER_DISCONNECTED */		{RMC_STATES_IDLE,						RMC_SM_ACT_NONE},
  /*RMC_MSG_BLE_NON_USER_DISCONNECTED*/	{RMC_STATES_ADVERTISING,		RMC_SM_ACT_DIRECT_ADV},
  /* RMC_MSG_TIMER_ADV_TIMEOUT*/				{RMC_STATES_CONNECTED,			RMC_SM_ACT_STOP_ADV},
  /* RMC_MSG_TIMER_DIRECT_ADV_TIMEOUT*/	{RMC_STATES_CONNECTED,			RMC_SM_ACT_NONE},
  /* RMC_MSG_TIMER_NO_ACT_TIMEOUT */		{RMC_STATES_IDLE,           RMC_SM_ACT_DISCONNECT_CONN},
  /* RMC_MSG_TIMER_SEC_TIMEOUT */				{RMC_STATES_IDLE,           RMC_SM_ACT_DISCONNECT_CONN},
  /* RMC_MSG_RECONN_FAIL */							{RMC_STATES_IDLE,           RMC_SM_ACT_NONE},
  /* RMC_MSG_POWER_UP */								{RMC_STATES_ADVERTISING,    RMC_SM_ACT_DIRECT_ADV},
  /* RMC_MSG_CLEAR_PIAR_KEY */  				{RMC_STATES_CONNECTED,    	RMC_SM_ACT_NONE},
  /* RMC_MSG_LOW_POWER     */   				{RMC_STATES_IDLE,    				RMC_SM_ACT_DISCONNECT_CONN},
	},
};

/* Action for rmc */

/* State machine action set array */
static const rmc_act_t rmc_act_set[] = {
	rmc_sm_act_none,
	rmc_sm_act_start_adv,
	rmc_sm_act_start_direct_adv,
	rmc_sm_act_stop_adv,
	rmc_sm_act_disconnect_conn,
	rmc_sm_act_send_key,
	rmc_sm_act_connected,
};

struct rmc_cb_t g_rmc_cb;

void rmc_sm_execute(u8_t evt)
{
	rmc_act_t      act_func;
	u8_t           action;
	u8_t           event;

	SYS_LOG_INF("rmc_sm_execute event=%d state=%d", evt, g_rmc_cb.rmc_status);

	/* get the event */
	event = evt;

	/* get action */
	action = rmc_State_table[g_rmc_cb.rmc_status][event][RMC_ACTION];

	/* set next state */
	g_rmc_cb.rmc_status = rmc_State_table[g_rmc_cb.rmc_status][event][RMC_NEXT_STATE];

	/* look up action set */
	act_func = rmc_act_set[action];

	/* if action set present */
	if (act_func != NULL) {
		/* execute action function in action set */
		act_func(event, NULL);
	} else {
		/* no action */
		rmc_sm_act_none(event, NULL);
	}
}

__init_once_text void rmc_sm_init(void)
{
	g_rmc_cb.rmc_status  = RMC_STATES_IDLE;
}

bool rmc_sm_state_is_activing(void)
{
	return (g_rmc_cb.rmc_status != RMC_STATES_CONNECTED);
}



