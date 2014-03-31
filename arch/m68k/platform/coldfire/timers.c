
/*
 *	timers.c -- generic ColdFire hardware timer support.
 *
 *	Copyright (C) 1999-2008, Greg Ungerer <gerg@snapgear.com>
 */


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/profile.h>
#include <linux/clocksource.h>
#include <asm/io.h>
#include <asm/traps.h>
#include <asm/machdep.h>
#include <asm/coldfire.h>
#include <asm/mcftimer.h>
#include <asm/mcfsim.h>


#define	FREQ	(MCF_BUSCLK / 16)
#define	TA(a)	(MCFTIMER_BASE1 + (a))

void coldfire_profile_init(void);

#if defined(CONFIG_M532x)
#define	__raw_readtrr	__raw_readl
#define	__raw_writetrr	__raw_writel
#else
#define	__raw_readtrr	__raw_readw
#define	__raw_writetrr	__raw_writew
#endif

static u32 mcftmr_cycles_per_jiffy;
static u32 mcftmr_cnt;

static irq_handler_t timer_interrupt;


static void init_timer_irq(void)
{
#ifdef MCFSIM_ICR_AUTOVEC
	
	writeb(MCFSIM_ICR_AUTOVEC | MCFSIM_ICR_LEVEL6 | MCFSIM_ICR_PRI3,
		MCF_MBAR + MCFSIM_TIMER1ICR);
	mcf_mapirq2imr(MCF_IRQ_TIMER, MCFINTC_TIMER1);

#ifdef CONFIG_HIGHPROFILE
	
	writeb(MCFSIM_ICR_AUTOVEC | MCFSIM_ICR_LEVEL7 | MCFSIM_ICR_PRI3,
		MCF_MBAR + MCFSIM_TIMER2ICR);
	mcf_mapirq2imr(MCF_IRQ_PROFILER, MCFINTC_TIMER2);
#endif
#endif 
}


static irqreturn_t mcftmr_tick(int irq, void *dummy)
{
	
	__raw_writeb(MCFTIMER_TER_CAP | MCFTIMER_TER_REF, TA(MCFTIMER_TER));

	mcftmr_cnt += mcftmr_cycles_per_jiffy;
	return timer_interrupt(irq, dummy);
}


static struct irqaction mcftmr_timer_irq = {
	.name	 = "timer",
	.flags	 = IRQF_DISABLED | IRQF_TIMER,
	.handler = mcftmr_tick,
};


static cycle_t mcftmr_read_clk(struct clocksource *cs)
{
	unsigned long flags;
	u32 cycles;
	u16 tcn;

	local_irq_save(flags);
	tcn = __raw_readw(TA(MCFTIMER_TCN));
	cycles = mcftmr_cnt;
	local_irq_restore(flags);

	return cycles + tcn;
}


static struct clocksource mcftmr_clk = {
	.name	= "tmr",
	.rating	= 250,
	.read	= mcftmr_read_clk,
	.mask	= CLOCKSOURCE_MASK(32),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};


void hw_timer_init(irq_handler_t handler)
{
	__raw_writew(MCFTIMER_TMR_DISABLE, TA(MCFTIMER_TMR));
	mcftmr_cycles_per_jiffy = FREQ / HZ;
	__raw_writetrr(mcftmr_cycles_per_jiffy - 1, TA(MCFTIMER_TRR));
	__raw_writew(MCFTIMER_TMR_ENORI | MCFTIMER_TMR_CLK16 |
		MCFTIMER_TMR_RESTART | MCFTIMER_TMR_ENABLE, TA(MCFTIMER_TMR));

	clocksource_register_hz(&mcftmr_clk, FREQ);

	timer_interrupt = handler;
	init_timer_irq();
	setup_irq(MCF_IRQ_TIMER, &mcftmr_timer_irq);

#ifdef CONFIG_HIGHPROFILE
	coldfire_profile_init();
#endif
}

#ifdef CONFIG_HIGHPROFILE

#define	PA(a)	(MCFTIMER_BASE2 + (a))

#define	PROFILEHZ	1013

irqreturn_t coldfire_profile_tick(int irq, void *dummy)
{
	
	__raw_writeb(MCFTIMER_TER_CAP | MCFTIMER_TER_REF, PA(MCFTIMER_TER));
	if (current->pid)
		profile_tick(CPU_PROFILING);
	return IRQ_HANDLED;
}


static struct irqaction coldfire_profile_irq = {
	.name	 = "profile timer",
	.flags	 = IRQF_DISABLED | IRQF_TIMER,
	.handler = coldfire_profile_tick,
};

void coldfire_profile_init(void)
{
	printk(KERN_INFO "PROFILE: lodging TIMER2 @ %dHz as profile timer\n",
	       PROFILEHZ);

	
	__raw_writew(MCFTIMER_TMR_DISABLE, PA(MCFTIMER_TMR));

	__raw_writetrr(((MCF_BUSCLK / 16) / PROFILEHZ), PA(MCFTIMER_TRR));
	__raw_writew(MCFTIMER_TMR_ENORI | MCFTIMER_TMR_CLK16 |
		MCFTIMER_TMR_RESTART | MCFTIMER_TMR_ENABLE, PA(MCFTIMER_TMR));

	setup_irq(MCF_IRQ_PROFILER, &coldfire_profile_irq);
}

#endif	
