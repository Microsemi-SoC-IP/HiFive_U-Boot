/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2018 Microsemi Corporation.
 * Padmarao Begari <Padmarao.Begari@microsemi.com>
 * Copyright (C) 2018 SiFive Inc.
 * Troy Benjegerdes <troy.benjegerdes@sifive.com>
 */


#include <common.h>
#include <asm/io.h>
#include <asm/encoding.h>

#if __riscv_xlen == 64 || __riscv64
	volatile uint64_t* mtime = (volatile uint64_t*)HIFIVE_BASE_MTIME;
	volatile uint64_t* timecmp = (volatile uint64_t*)HIFIVE_BASE_TIMECMP;
#else
 	volatile uint32_t* mtime = (volatile uint32_t*)HIFIVE_BASE_MTIME;
	volatile uint32_t* timecmp = (volatile uint32_t*)HIFIVE_BASE_TIMECMP;
#endif

volatile ulong starttime = 0UL;

int timer_init(void)
{
        starttime = *mtime;
        return 0;
}

/*
 * return difference between timer ticks and base
 */
ulong get_timer(ulong base)
{
        ulong now = *mtime;
	return (now - starttime)/(CONFIG_SYS_HZ_CLOCK/CONFIG_SYS_HZ) - base;
}

/* delay x useconds */
void __udelay(unsigned long usec)
{
	/* as long as 1 tick is 1 usec, optimize.. */
	/* TODO: audit this code for bad behavior on overflow */
	int start = get_timer(0);
	while ((get_timer(0) - usec) < start)
            ;
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On RISC-V it just returns the timer value (uSec since boot)
 */
unsigned long long get_ticks(void)
{
	return get_timer(0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On RISC-V it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ_CLOCK;
}
