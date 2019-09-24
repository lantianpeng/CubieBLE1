/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <soc.h>
#include <irc.h>
#include "irc_acts.h"

#define SYS_LOG_DOMAIN "IRKEY"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_INPUT_DEV_LEVEL
#include <logging/sys_log.h>

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct acts_irc_config *)(dev)->config->config_info)

int irc_acts_output_key(struct device *dev, struct input_value *val)
{
	unsigned int key;
	const struct acts_irc_config *cfg = DEV_CFG(dev);
	struct acts_irc_data *drv_data = dev->driver_data;

	key = irq_lock();
	switch (cfg->coding_mode) {
	case TC9012_MODE:
	case NEC_MODE:
		val->code &= 0xFF;
		val->code = ((val->code ^ 0xFF) << 8) | val->code;
		break;
	}

	drv_data->key_state = val->value;
	drv_data->key_code = val->code;

	if ((drv_data->key_state == IR_KEY_DOWN) && (drv_data->busy == 0)) {
		if (!drv_data->irc_rx_enable) {
			device_busy_set(dev);
			/* reset irc controller */
			acts_reset_peripheral(cfg->reset_id);
			/* enable IRC TX clock */
			acts_clock_peripheral_enable(cfg->clock_id);
		}
		drv_data->old_mfp_ctl = sys_read32(IR_TX_IO_CTL);
		sys_write32(0x0E | GPIO_CTL_PADDRV_LEVEL(1), IR_TX_IO_CTL);  /* tmp do it */

		sys_write32(cfg->customer_code, IRC_TX_CC);
		sys_write32(drv_data->key_code, IRC_TX_KDC);
		sys_write32(cfg->coding_mode | IRC_CTRL_IIE | IRC_CTRL_IRE, IRC_TX_CTL);
		/* modulator enable*/
		sys_write32(sys_read32(IRC_TX_CTL) | IRC_CTRL_TX_MODE_FREQ | IRC_CTRL_TX_MODE_EN, IRC_TX_CTL);
		/* release */
		sys_write32(sys_read32(IRC_TX_CTL) | IRC_CTRL_TX_RELEASE, IRC_TX_CTL);
		drv_data->busy = 1;
		drv_data->pre_key_code = drv_data->key_code;
	}

	irq_unlock(key);

	return 0;
}
#ifdef CONFIG_IRC_RX
void irc_input_acts_report_key(struct acts_irc_data *drv_data, int key_code, int value)
{
	struct input_value val;

	if (drv_data->notify) {
		val.type = EV_KEY;
		val.code = key_code;
		val.value = value;

		SYS_LOG_DBG("report key_code 0x%x value 0x%x, key code 0x%x",
			key_code, value, val.code);

		drv_data->notify(NULL, &val);
	}
}
#endif
void irc_acts_isr(struct device *dev)
{
	const struct acts_irc_config *cfg = DEV_CFG(dev);
	struct acts_irc_data *drv_data = dev->driver_data;

	if (sys_read32(IRC_TX_STA) & 0x01) {
		if (drv_data->key_state == IR_KEY_DOWN) {
			if (drv_data->pre_key_code != drv_data->key_code)
				sys_write32(drv_data->key_code, IRC_TX_KDC);
			else
				sys_write32(sys_read32(IRC_TX_CTL) | IRC_CTRL_TX_RPC_EN, IRC_TX_CTL);

			sys_write32(sys_read32(IRC_TX_CTL) | IRC_CTRL_TX_RELEASE, IRC_TX_CTL);
			drv_data->busy = 1;
		} else {
			/* TODO */
			drv_data->busy = 0;
			sys_write32(0, IRC_TX_CTL);
			sys_write32(drv_data->old_mfp_ctl, IR_TX_IO_CTL);  /* tmp do it */
			if (!drv_data->irc_rx_enable) {
				acts_clock_peripheral_disable(cfg->clock_id);
				device_busy_clear(dev);
			}

			SYS_LOG_DBG("IR_TX_IO_CTL : 0x%x", sys_read32(IR_TX_IO_CTL));
		}

		drv_data->pre_key_code = drv_data->key_code;
		sys_write32(0x1, IRC_TX_STA);
	}
#ifdef CONFIG_IRC_RX
	if ((sys_read32(IRC_RX_STA) >> IRC_RX_IIP) & 0x01) {
		/* check protocol_ok_flag */
#ifdef USE_SOFT_MODE
		if (((sys_read32(IRC_RX_STA) >> IRC_RX_UCMP) & 0x1) == 0) {
#else
		if ((sys_read32(IRC_RX_STA) >> IRC_RX_DET_PRO_PD) & 0x1) {
#endif
			u32_t rx_cc, rx_kdc, rx_protocol;
#ifdef USE_SOFT_MODE
			rx_protocol = RX_PROTOCOL;
#else
			rx_protocol = (sys_read32(IRC_RX_STA) >> IRC_RX_PROTOCOL)&0x3;
#endif
			SYS_LOG_DBG("prot %d,rep %d, 0x%x, 0x%x",
									(sys_read32(IRC_RX_STA) >> IRC_RX_PROTOCOL)&0x3,
									(sys_read32(IRC_RX_STA) >> IRC_RX_RCD) & 0x1,
									sys_read32(IRC_RX_CC),
									sys_read32(IRC_RX_KDC));

			rx_cc = sys_read32(IRC_RX_CC) >> 16;
			rx_kdc = sys_read32(IRC_RX_KDC);
			switch (rx_protocol) {
			case TC9012_MODE:
			case NEC_MODE:
				rx_cc &= 0xFFFF;
				rx_kdc &= 0xFF;
				break;
			}

			irc_input_acts_report_key(drv_data, rx_cc, rx_kdc);
		} else {
			SYS_LOG_DBG("unknow protocol");
		}

		/* clear pending */
		sys_write32(sys_read32(IRC_RX_STA), IRC_RX_STA);
		/* IO_WRITE(IRC_RX_STA, IO_READ(IRC_RX_STA)); */

	}
#endif
}
#ifdef CONFIG_IRC_RX
void irc_input_acts_register_notify(struct device *dev, input_notify_t notify)
{
	struct acts_irc_data *drv_data = dev->driver_data;

	SYS_LOG_DBG("register notify 0x%x", (u32_t)notify);

	drv_data->notify = notify;
}

void irc_input_acts_unregister_notify(struct device *dev, input_notify_t notify)
{
	struct acts_irc_data *drv_data = dev->driver_data;

	SYS_LOG_DBG("unregister notify 0x%x", (u32_t)notify);

	drv_data->notify = NULL;
}


void irc_input_acts_enable(struct device *dev)
{
	const struct acts_irc_config *cfg = DEV_CFG(dev);
	struct acts_irc_data *drv_data = dev->driver_data;

	SYS_LOG_DBG("enable irc input");

	/* reset irc controller */
	acts_reset_peripheral(cfg->reset_id);
	/* enable IRC clock */
	acts_clock_peripheral_enable(cfg->clock_id);
	/* enable IRC RX clock */
	acts_clock_peripheral_enable(cfg->rx_clock_id);
#ifdef USE_ANA_INPUT
	sys_write32(0x91, IR_RX_IO_CTL);
#else
	sys_write32(0x8f, IR_RX_IO_CTL);
#endif

	sys_write32(sys_read32(IRC_RX_STA), IRC_RX_STA);

#ifdef USE_ANA_INPUT
	sys_write32(sys_read32(IRC_ANA_CTL) | (0x1<<1), IRC_ANA_CTL);
	sys_write32(sys_read32(IRC_ANA_CTL) | (0x1<<2), IRC_ANA_CTL);
	k_busy_wait(1000);
	sys_write32(sys_read32(IRC_ANA_CTL) & ~(0x1<<2), IRC_ANA_CTL);
	sys_write32(sys_read32(IRC_ANA_CTL) | (0x1<<0), IRC_ANA_CTL);
	sys_write32(sys_read32(IRC_ANA_CTL) | (0x2aa0), IRC_ANA_CTL);
#endif

	sys_write32(sys_read32(IRC_RX_CTL) | IRC_RX_CTRL_IIE | IRC_RX_CTRL_IRE, IRC_RX_CTL);
#ifdef USE_ANA_INPUT
	/* data input select analog,enable demodulator */
	sys_write32(sys_read32(IRC_RX_CTL) & ~(0x1<<16), IRC_RX_CTL);
	sys_write32(sys_read32(IRC_RX_CTL) | (0x3<<18), IRC_RX_CTL);
#else
	sys_write32(sys_read32(IRC_RX_CTL) | (0x1<<16), IRC_RX_CTL);
#endif

#ifdef USE_SOFT_MODE
	sys_write32(sys_read32(IRC_RX_CTL) & ~(0x1<<17), IRC_RX_CTL);
	sys_write32(sys_read32(IRC_RX_CTL) | (0x1<<20), IRC_RX_CTL);
	sys_write32(sys_read32(IRC_RX_CTL) | RX_PROTOCOL, IRC_RX_CTL);
	sys_write32(RX_CUSTOMER_CODE, IRC_RX_CC);
#else
	sys_write32(sys_read32(IRC_RX_CTL) | (0x1<<17), IRC_RX_CTL);
#endif
	device_busy_set(dev);
	drv_data->irc_rx_enable = 1;

}

void irc_input_acts_disable(struct device *dev)
{
	const struct acts_irc_config *cfg = DEV_CFG(dev);
	struct acts_irc_data *drv_data = dev->driver_data;

	SYS_LOG_DBG("disable irc input");

	acts_clock_peripheral_disable(cfg->rx_clock_id);
	acts_clock_peripheral_disable(cfg->clock_id);
	acts_reset_peripheral(cfg->reset_id);

	sys_write32(0x0, IR_RX_IO_CTL);

	drv_data->irc_rx_enable = 0;
	device_busy_clear(dev);
}

#endif

const struct irc_driver_api irc_acts_drv_api_funcs = {
#ifdef CONFIG_IRC_RX
	.enable = irc_input_acts_enable,
	.disable = irc_input_acts_disable,
	.register_notify = irc_input_acts_register_notify,
	.unregister_notify = irc_input_acts_unregister_notify,
#endif
	.irc_output = irc_acts_output_key,
};

int irc_acts_init(struct device *dev);

struct acts_irc_data  irc_acts_drv_data;

const struct acts_irc_config irc_acts_dev_cfg_info = {
	.base = IRC_REG_BASE,
	.customer_code = CUSTOMER_CODE,
	.coding_mode = NEC_MODE,
	.clock_id = CLOCK_ID_IRC_CLK,
	.reset_id = RESET_ID_IRC,
	.rx_clock_id = CLOCK_ID_IRC_RXCLK,
};


DEVICE_AND_API_INIT(irc_acts, CONFIG_IRC_ACTS_DEV_NAME,
		    irc_acts_init,
		    &irc_acts_drv_data, &irc_acts_dev_cfg_info,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &irc_acts_drv_api_funcs);

int irc_acts_init(struct device *dev)
{
	IRQ_CONNECT(IRQ_ID_IRC, 0,
		    irc_acts_isr, DEVICE_GET(irc_acts), 0);
	irq_enable(IRQ_ID_IRC);

	return 0;
}
