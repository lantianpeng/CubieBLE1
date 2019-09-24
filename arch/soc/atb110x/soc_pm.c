/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file soc suspend interface for Actions SoC
 */

#include <kernel.h>
#include <init.h>
#include <power.h>
#include <soc.h>

#define SYS_LOG_DOMAIN "power"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#include <logging/sys_log.h>

#if CONFIG_DEEPSLEEP

#define _SCB_SCR 0xE000ED10
#define _SCB_SCR_SLEEPDEEP (1 << 2)

enum power_states {
	SYS_POWER_STATE_CPU_SLEEP,       /* CPU sleep state */
	SYS_POWER_STATE_DEEP_SLEEP,    /* DEEP SLEEP state including cpu and pmu */
	SYS_POWER_STATE_MAX
};

static int pm_state;
static __attribute__((used, section(".ram_retention_data")))  char sleep_32k = 1;
__attribute__((used, section(".ram_retention_data"))) u8_t deepsleep_disable_vd12;
static __ramfunc void request_32M_XTAL(bool ena)
{
#if CONFIG_DEEPSLEEP_SWITCH_32M
	if (ena) {
		sys_write32(sys_read32(ACT_32M_CTL) | (1 << ACT_32M_CTL_XTAL32M_EN), ACT_32M_CTL);
		while ((sys_read32(ACT_32M_CTL) & (0x1<<1)) == 0)
			;
	} else {
		sys_write32(sys_read32(ACT_32M_CTL) & ~(1 << ACT_32M_CTL_XTAL32M_EN), ACT_32M_CTL);
		acts_delay_us(100);
	}
#endif
}

static void request_vd12(bool ena)
{
	/*always true*/
}

static __attribute__((used, section(".ram_retention_code"))) void request_rc_3M(bool ena)
{
	if (ena) {
		request_vd12(true);
		sys_write32(sys_read32(ACT_3M_CTL) | (1 << ACT_3M_CTL_RC3MEN), ACT_3M_CTL);
		while ((sys_read32(ACT_3M_CTL) & (1 << ACT_3M_CTL_RC3M_OK)) == 0)
			;
	} else {
		sys_write32(sys_read32(ACT_3M_CTL) & ~(1 << ACT_3M_CTL_RC3MEN), ACT_3M_CTL);
		request_vd12(false);
	}
}

static void request_vd12_largebias(bool ena)
{
	if (ena)
		sys_write32(sys_read32(VD12_CTL) | (1 << VD12_CTL_VD12_LGBIAS_EN),
									VD12_CTL);
	else
		sys_write32(sys_read32(VD12_CTL) & ~(1 << VD12_CTL_VD12_LGBIAS_EN),
									VD12_CTL);
}


static void set_deep_sleep_clock(void)
{
	/* If hcl32k is enabled, deepsleep should select 3M clock */
	if (sys_read32(HCL32K_CTL) & (1 << HCL32K_CTL_HCL_32K_EN))
		sleep_32k = 0;

	if (sleep_32k == 0)
		sys_write32(sys_read32(CMU_SYSCLK) | (1 << CMU_SYSCLK_SLEEP_HFCLK_SEL), CMU_SYSCLK);
	else
		sys_write32(sys_read32(CMU_SYSCLK) & ~(1 << CMU_SYSCLK_SLEEP_HFCLK_SEL), CMU_SYSCLK);
}

static __ramfunc void switch_cpu_clock_3M(void)
{
	request_rc_3M(true);
	sys_write32(sys_read32(CMU_SYSCLK) & ~CMU_SYSCLK_CPU_CLK_SEL_MASK, CMU_SYSCLK);
	system_core_clock = SYSTEM_CLOCK_3M;
}

static void enable_wake_event(void)
{
}

static void __set_primask(u32_t primask)
{
	register u32_t __reg_primask __asm("primask");
	__reg_primask = (primask);
}

void suspend_devices(void)
{
	/* call devices suspend */
	set_devices_state(DEVICE_PM_SUSPEND_STATE);

	/* disable unused clock */
	/*acts_clock_peripheral_disable(0); */

	/* disable jtag_en */
	sys_write32(0, JTAG_EN);
}

void resume_devices(void)
{
#ifndef CONFIG_USE_JTAG_IO_FOR_KEY
	/* enable jtag_en */
	sys_write32(((u32_t)1<<31), JTAG_EN);
#endif

	/* restore clock */
	/*acts_clock_peripheral_enable(0); */

	/* call devices resume */
	set_devices_state(DEVICE_PM_ACTIVE_STATE);
}

static __ramfunc void deep_sleep_exit(void)
{

	/* enalbe 32M xtal and select 32M as hfclk */
	request_32M_XTAL(true);
	sys_write32((sys_read32(CMU_SYSCLK) & ~CMU_SYSCLK_CPU_CLK_SEL_MASK) | (0x2), CMU_SYSCLK);
	system_core_clock = SYSTEM_CLOCK_32M;
	request_rc_3M(false);

	/* disable vdd12 large bias */
	request_vd12_largebias(false);

	/* Clear disable bit.*/
	sys_write32(sys_read32(_SCB_SCR) & ~_SCB_SCR_SLEEPDEEP, _SCB_SCR);

	/* Turn on peripherals and restore device states as necessary */
	resume_devices();
}

static __attribute__((used, section(".ram_retention_code"))) void delay_cpu_cycle(uint32_t cycles)
{
	volatile char delay_cnt;

	for (delay_cnt = 0; delay_cnt < cycles; delay_cnt++)
		;
}

#define RAM2_START 0x20004000
#define RAM6_START 0x20008000
__attribute__((used, section(".ram_retention_code"))) void _sys_lower_wfi(void)
{
#ifndef CONFIG_SPI0_XIP
	u32_t ram_retention_start = (u32_t)Image$$RAM_RETENTION$$Base;
	u32_t ram_retention_end = (u32_t)Image$$RAM_RETENTION$$Limit;
	/* {keep ram2, keep ram3, keep ram4, keep ram5} */
	u32_t ram_ctl_val[] = {0xfb, 0xf7, 0xef, 0xdf};
	/* {keep ram2+3, keep ram3+4, keep ram4+5, keep ram5+6} */
	u32_t ram_ctl_cross_val[] = {0xf3, 0xe7, 0xcf, 0x9f};
	u32_t ram_start_index, ram_end_index;

	/* set ram retention */
	if ((ram_retention_start >= RAM2_START) && (ram_retention_start < RAM6_START)) {
		ram_start_index = ((ram_retention_start - RAM2_START) & 0xf000) >> 12;
		ram_end_index = ((ram_retention_end - RAM2_START) & 0xf000) >> 12;
		if (ram_start_index == ram_end_index)
			sys_write32(ram_ctl_val[ram_start_index] | 0x50000, RAM_CTL);
		else
			sys_write32(ram_ctl_cross_val[ram_start_index] | 0x50000, RAM_CTL);
	}
#else
	sys_write32(0xfe | 0x50000, RAM_CTL);
	sys_write32(sys_read32(CMU_DEVCLKEN) & ~(0x1<<CLOCK_ID_SPICACHE), CMU_DEVCLKEN);
	sys_write32(sys_read32(CMU_DEVCLKEN) & ~(0x1<<CLOCK_ID_SPI0), CMU_DEVCLKEN);
#endif

	if (sleep_32k == 0)
		sys_write32(sys_read32(VD12_CTL) & ~((1<<VD12_CTL_VD12_LGBIAS_EN) | (1<<VD12_CTL_VD12PD_EN)), VD12_CTL);
	else {
		/* switch clock to 32k and wait for 32k clock stable */
		sys_write32((sys_read32(CMU_SYSCLK) & ~CMU_SYSCLK_CPU_CLK_SEL_MASK) | (0x1), CMU_SYSCLK);
		delay_cpu_cycle(5);

		/* disable 3M RC clock, and configure vd12 for power_save */
		request_rc_3M(false);
		sys_write32(sys_read32(VD12_CTL) & ~((1<<VD12_CTL_VD12_LGBIAS_EN) | (1<<VD12_CTL_VD12PD_EN)), VD12_CTL);
		/* disable vd12 */
		if (deepsleep_disable_vd12)
			sys_write32(sys_read32(VD12_CTL) & ~(1<<VD12_CTL_VD12_EN), VD12_CTL);
	}

	__asm("wfi");

	/* restore ram retention */
	sys_write32(0, RAM_CTL);

#ifdef CONFIG_SPI0_XIP
	sys_write32(sys_read32(CMU_DEVCLKEN) | (0x1<<CLOCK_ID_SPI0), CMU_DEVCLKEN);
	sys_write32(sys_read32(CMU_DEVCLKEN) | (0x1<<CLOCK_ID_SPICACHE), CMU_DEVCLKEN);
#endif

	/* restore vd12 & RC_3M by software if pmu DON'T enter s3 */
	sys_write32(sys_read32(VD12_CTL) | ((1<<VD12_CTL_VD12_LGBIAS_EN) | (1<<VD12_CTL_VD12PD_EN)), VD12_CTL);
	sys_write32(sys_read32(VD12_CTL) | (1<<VD12_CTL_VD12_EN), VD12_CTL);
	switch_cpu_clock_3M();
}

static __ramfunc void _sys_soc_set_power_state(enum power_states state)
{
	switch (state) {
	case SYS_POWER_STATE_CPU_SLEEP:
		break;
	case SYS_POWER_STATE_DEEP_SLEEP:
		/* Enable deep sleep in CPU. */
		sys_write32(sys_read32(_SCB_SCR) | _SCB_SCR_SLEEPDEEP, _SCB_SCR);

		/* select deepsleep cock, switch clock to 3M, disable 32M request */
		set_deep_sleep_clock();
		switch_cpu_clock_3M();
		request_32M_XTAL(false);

		/* reduce vdd before enter deepsleep */
		acts_set_vdd(dp_vdd_val);
		acts_delay_us(100);

		_sys_lower_wfi();

		/* restore vdd after exit deepsleep */
		acts_set_vdd(normal_vdd_val);
		acts_delay_us(100);
		break;
	default:
		break;
	}
}

__ramfunc int deep_sleep_entry(s32_t ticks)
{
	SYS_LOG_DBG("enter ds");

	/* Save device states and turn off peripherals as necessary */
	suspend_devices();

	enable_wake_event();

	_timer_deepsleep_enter(ticks);

	_sys_soc_set_power_state(SYS_POWER_STATE_DEEP_SLEEP);

	/*
	 * At this point system has woken up from
	 * deep sleep.
	 */
	deep_sleep_exit();

	_timer_deepsleep_exit();

	__set_primask(0);

	SYS_LOG_DBG("exit ds");

	return SYS_PM_DEEP_SLEEP;
}

static int low_power_state_entry(s32_t ticks)
{
	_sys_soc_set_power_state(SYS_POWER_STATE_CPU_SLEEP);

	return SYS_PM_NOT_HANDLED;
}

static int check_pm_policy(s32_t ticks)
{
	int policy = 2;
	int power_states[] = {SYS_POWER_STATE_MAX, SYS_POWER_STATE_CPU_SLEEP,
		SYS_POWER_STATE_DEEP_SLEEP};

	/*
	 * If deep sleep was selected, we need to check if any device is in
	 * the middle of a transaction
	 *
	 * Use device_busy_check() to check specific devices
	 */
	if ((policy == 2) && application_wake_lock) {
		/* Applications are busy - do CPU LPS instead */
		policy = 1;
	}

	if ((policy == 2) && device_any_busy_check()) {
		/* Devices are busy - do CPU LPS instead */
		policy = 1;
	}

	return power_states[policy];
}

int _sys_soc_suspend_new(s32_t ticks)
{
	int ret = SYS_PM_NOT_HANDLED;

	if ((ticks != K_FOREVER) && (ticks < CONFIG_DEEPSLEEP_TICK_THRESH))
		return SYS_PM_NOT_HANDLED;

#if !CONFIG_DEEPSLEEP
	return SYS_PM_NOT_HANDLED;
#endif

	pm_state = check_pm_policy(ticks);

	switch (pm_state) {
	case SYS_POWER_STATE_CPU_SLEEP:
		/* Do CPU sleep operations */
		ret = low_power_state_entry(ticks);
		break;
	case SYS_POWER_STATE_DEEP_SLEEP:
		/* Do deep sleep operations */
		ret = deep_sleep_entry(ticks);
		break;
	default:
		/* No PM operations */
		SYS_LOG_DBG("\nNo PM operations done");
		ret = SYS_PM_NOT_HANDLED;
		break;
	}

	return ret;
}

void patch_sys_soc_suspend(void)
{
	p_sys_soc_suspend = _sys_soc_suspend_new;
}

/* replace wfi in idle with nop(46c0 (mov r8, r8))*/
CODE_PATCH_REGISTER(idle_wfi, 0x46c0b662, 0x22824);
#else

void idle_new(void *unused1, void *unused2, void *unused3)
{
	for (;;)
		;
}
FUNCTION_PATCH_REGISTER(idle, idle_new, idle);
#endif

#include <watchdog.h>

struct reboot_reason {
	u32_t magic;
	u32_t reason;
};

void sys_pm_reboot(int type)
{
	struct device *wdg;
	struct wdt_config config;
	struct reboot_reason *reason;

	SYS_LOG_INF("system reboot, type %d!", type);

	wdg = device_get_binding(CONFIG_WDG_ACTS_DEV_NAME);
	if (!wdg) {
		SYS_LOG_ERR("cannot found watchdog device, failed to reboot");
		while (1)
			;
	}

	config.timeout = 100;
	config.mode = WDT_MODE_RESET;

	irq_lock();

	reason = (struct reboot_reason *)REBOOT_REASON_ADDR;
	reason->magic = REBOOT_REASON_MAGIC;
	reason->reason = type & ~0xff;

	wdt_set_config(wdg, &config);
	wdt_enable(wdg);

	while (1)
		;

	/* never return... */
}
