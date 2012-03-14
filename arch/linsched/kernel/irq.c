#include <linux/interrupt.h>
#include <asm/hardirq.h>
#include <linux/sched.h>

unsigned long linsched_irq_flags = ARCH_IRQ_ENABLED;

static struct softirq_action softirq_vec[NR_SOFTIRQS];

cpumask_t linsched_cpu_softirq_raised;

irq_cpustat_t irq_stat[NR_CPUS];

#define __local_bh_enable(cnt) sub_preempt_count(cnt)
#define __local_bh_disable(cnt) add_preempt_count(cnt)

void open_softirq(int nr, void (*action)(struct softirq_action *))
{
	softirq_vec[nr].action = action;
}

void do_softirq(void)
{
	unsigned pending = local_softirq_pending();
	unsigned allowed = 1 << SCHED_SOFTIRQ | 1 << HRTIMER_SOFTIRQ;
	struct softirq_action *h;
	unsigned long flags;

	__local_bh_disable(SOFTIRQ_OFFSET);

	local_irq_save(flags);
	while ((pending = local_softirq_pending())) {
		/*
		 * For now we only support SCHED_SOFTIRQ
		 * and HRTIMER_SOFTIRQ
		 */
		BUG_ON((~allowed) & pending);

		cpumask_clear_cpu(smp_processor_id(),
				&linsched_cpu_softirq_raised);
		set_softirq_pending(0);
		h = softirq_vec;
		do {
			if (pending & 1)
				h->action(h);
			h++;
			pending >>= 1;
		} while (pending);
	}
	local_irq_restore(flags);

	__local_bh_enable(SOFTIRQ_OFFSET);
}

void raise_softirq_irqoff(unsigned int nr)
{
	if (nr != SCHED_SOFTIRQ && nr != HRTIMER_SOFTIRQ)
		return;

	cpumask_set_cpu(smp_processor_id(), &linsched_cpu_softirq_raised);
	 __raise_softirq_irqoff(nr);

}
void raise_softirq(unsigned int nr)
{
	unsigned long flags;

	local_irq_save(flags);
	raise_softirq_irqoff(nr);
	local_irq_restore(flags);
}

void irq_enter(void)
{
	__irq_enter();
}

void irq_exit(void)
{
	account_system_vtime(current);
	trace_hardirq_exit();
	sub_preempt_count(IRQ_EXIT_OFFSET);
	if (!in_interrupt() && local_softirq_pending())
		do_softirq();
}

void local_bh_enable_ip(unsigned long ip)
{
	WARN_ON_ONCE(in_irq() || irqs_disabled());
	sub_preempt_count(SOFTIRQ_DISABLE_OFFSET - 1);

	if (unlikely(!in_interrupt() && local_softirq_pending()))
		do_softirq();
	dec_preempt_count();

	preempt_check_resched();
}

void local_bh_enable(void)
{
	local_bh_enable_ip((unsigned long)__builtin_return_address(0));
}

void local_bh_disable(void)
{
	add_preempt_count(SOFTIRQ_DISABLE_OFFSET);
}

void add_preempt_count(int val)
{
	preempt_count() += val;
}

void sub_preempt_count(int val)
{
	BUG_ON(val > preempt_count());
	preempt_count() -= val;
}
