/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "errno.h"
#include <string.h>
#include <kernel.h>
#include <init.h>
#include <ksched.h>
#include <timeout_q.h>
#include <misc/printk.h>
#include <soc.h>

int k_sys_work_q_init(struct device *dev);
SYS_INIT(k_sys_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

int new_k_delayed_work_cancel(struct k_delayed_work *work)
{
	int key = irq_lock();

	if (!work->work_q) {
		irq_unlock(key);
		return -EINVAL;
	}

	if (k_work_pending(&work->work)) {
		/* Remove from the queue if already submitted */
		if (!k_queue_remove(&work->work_q->queue, &work->work)) {
			irq_unlock(key);
			return -EINVAL;
		}
	} else {
		_abort_timeout(&work->timeout);
	}

	/* Detach from workqueue */
	work->work_q = NULL;

	atomic_clear_bit(work->work.flags, K_WORK_STATE_PENDING);
	irq_unlock(key);

	return 0;
}

FUNCTION_PATCH_REGISTER(k_delayed_work_cancel, new_k_delayed_work_cancel, k_delayed_work_cancel);
