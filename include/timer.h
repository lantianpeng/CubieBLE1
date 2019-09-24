/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for timer drivers
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

#define CONFIG_TIMER1_ACTS_DRV_NAME "TIMER_1"
#define CONFIG_TIMER_ACTS_INIT_PRIORITY 20

/**
 * @brief TIMER Driver APIs
 * @defgroup timer_interface TIMER Driver APIs
 * @{
 */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*timer_irq_callback_t)(struct device *dev);

/** @brief Driver API structure. */
struct timer_driver_api {
	/** start timer */
	int (*start)(struct device *dev, s32_t duration, s32_t period);
		
	/** stop timer */
	int (*stop)(struct device *dev);

	/** Set the callback function */
	void (*irq_callback_set)(struct device *dev, timer_irq_callback_t cb);
	
	/** user data set */
	void (*user_data_set)(struct device *dev, void *user_data);
	
	/** user data get */
	void *(*user_data_get)(struct device *dev);
};

/**
 * @brief timer start
 *
 * @param dev Device struct
 * @param duration  Initial timer duration (in milliseconds).
 * @param period    Timer period (in milliseconds).
 *
 * @return 0 if successful, failed otherwise
 */
static inline int timer_start(struct device *timer, s32_t duration, s32_t period)
{
	const struct timer_driver_api *api = timer->driver_api;

	return api->start(timer, duration, period);
}

/**
 * @brief stop timer
 *
 * @param dev Device struct
 *
 * @return 0 if successful, failed otherwise
 */
static inline int timer_stop(struct device *timer)
{
	const struct timer_driver_api *api = timer->driver_api;

	return api->stop(timer);
}

/**
 * @brief Enable callback(s) for a single pin.
 * @param timer Pointer to the device structure for the driver instance.
 * @param cb callback function.
 * @return 0 if successful, negative errno code on failure.
 */
static inline void timer_set_callback(struct device *timer, timer_irq_callback_t cb)
{
	const struct timer_driver_api *api = timer->driver_api;

	api->irq_callback_set(timer, cb);
}

/**
 * @brief Associate user-specific data with a timer.
 *
 * This routine records the @a user_data with the @a timer, to be retrieved
 * later.
 *
 * It can be used e.g. in a timer handler shared across multiple subsystems to
 * retrieve data specific to the subsystem this timer is associated with.
 *
 * @param timer     Address of timer.
 * @param user_data User data to associate with the timer.
 *
 * @return N/A
 */
static inline void timer_user_data_set(struct device *timer,
					 void *user_data)
{
	const struct timer_driver_api *api = timer->driver_api;

	api->user_data_set(timer, user_data);
}

/**
 * @brief Retrieve the user-specific data from a timer.
 *
 * @param timer     Address of timer.
 *
 * @return The user data.
 */
static inline void *timer_user_data_get(struct device *timer)
{
	const struct timer_driver_api *api = timer->driver_api;

	return api->user_data_get(timer);
}


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __TIMER_H__ */
