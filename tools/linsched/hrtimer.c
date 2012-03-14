/* LinSched -- The Linux Scheduler Simulator
 * Copyright (C) 2008  John M. Calandrino
 * E-mail: jmc@cs.unc.edu
 *
 * This file contains Linux variables and functions that have been "defined
 * away" or exist here in a modified form to avoid including an entire Linux
 * source file that might otherwise lead to a "cascade" of dependency issues.
 * It also includes certain LinSched variables to which some Linux functions
 * and definitions now map.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING); if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "linsched.h"
#include <linux/tick.h>
#include <linux/interrupt.h>

#include "load_balance_score.h"
#include "nohz_tracking.h"
#include "sanity_check.h"

static int linsched_hrt_set_next_event(unsigned long evt,
				       struct clock_event_device *d);
static void linsched_hrt_set_mode(enum clock_event_mode mode,
				  struct clock_event_device *d);
static void linsched_hrt_broadcast(const struct cpumask *mask);
static cycle_t linsched_source_read(struct clocksource *cs);

/* Some assumptions fail if we stay at time 0 during setup, so just dodge it */
u64 current_time = 100;
static u64 next_event[NR_CPUS];
static struct clock_event_device linsched_hrt[NR_CPUS];

static struct clock_event_device linsched_hrt_base = {
	.name = "linsched-events",
	.features = CLOCK_EVT_FEAT_ONESHOT,
	.max_delta_ns = 1000000000L,
	.min_delta_ns = 5000,
	.mult = 1,
	.shift = 0,
	.rating = 100,
	.irq = -1,
	.cpumask = NULL,
	.set_next_event = linsched_hrt_set_next_event,
	.set_mode = linsched_hrt_set_mode,
	.broadcast = linsched_hrt_broadcast
};

/* We need a clocksource other than jiffies in order to run
 * HIGH_RES_TIMERS, so replace the default. jiffies will then be
 * updated by kernel/time/tick-common.c and
 * kernel/time/tick-sched.c */
static struct clocksource linsched_hrt_source = {
	.name = "linsched-clocksource",
	.rating = 100,
	.mask = (cycle_t) -1,
	.mult = 1,
	.shift = 0,
	.flags = CLOCK_SOURCE_VALID_FOR_HRES | CLOCK_SOURCE_IS_CONTINUOUS,
	.read = linsched_source_read,
};

void linsched_init_hrtimer(void)
{
	long i;

	for (i = 0; i < nr_cpu_ids; i++) {
		struct clock_event_device *dev = &linsched_hrt[i];
		memcpy(dev, &linsched_hrt_base,
		       sizeof(struct clock_event_device));
		dev->cpumask = cpumask_of(i);

		next_event[i] = KTIME_MAX;

		linsched_change_cpu(i);
		clockevents_register_device(dev);
	}
	/* Normally this would be called when looking through
	 * clocksources, but we're not using the entire clocksource
	 * infrastructure at the moment */
	tick_clock_notify();
	for (i = 0; i < nr_cpu_ids; i++) {
		linsched_change_cpu(i);
		hrtimer_run_pending();
	}
	linsched_change_cpu(0);
}

void linsched_current_handler(void)
{
	struct task_struct *old;

	do {
		struct task_data *td = task_thread_info(current)->td;
		old = current;

		if (td && td->handle_task) {
			td->handle_task(current, td->data);
		}
		/* we keep calling handle_task so that tasks
		 * can have smaller than clock gran runtimes if they want */
		linsched_check_resched();
	} while (current != old);

	/* we should always be done by this point */
	BUG_ON(need_resched());
}

extern cpumask_t linsched_cpu_softirq_raised;
void process_all_softirqs(void)
{
	int cpu, old_cpu = smp_processor_id();

	if (cpumask_empty(&linsched_cpu_softirq_raised))
		return;

	while (!cpumask_empty(&linsched_cpu_softirq_raised)) {
		cpu = cpumask_first(&linsched_cpu_softirq_raised);
		linsched_change_cpu(cpu);
		do_softirq();

		/* we may have pulled something over that wants to run */
		linsched_current_handler();
	}

	BUG_ON(irqs_disabled());
	linsched_change_cpu(old_cpu);
}

DECLARE_PER_CPU(struct tick_sched, tick_cpu_sched);
void linsched_check_idle_cpu(void)
{
	int cpu = smp_processor_id();
	struct tick_sched *ts = &per_cpu(tick_cpu_sched, cpu);

	if (ts->inidle && !idle_cpu(cpu)) {
		tick_nohz_idle_exit();
	}
}

/* Run a simulation for some number of ticks. Each tick,
 * scheduling and load balancing decisions are made. Obviously, we
 * could create tasks, change priorities, etc., at certain ticks
 * if we desired, rather than just running a simple simulation.
 * (Tasks can also be removed by having them exit.)
 */
void linsched_run_sim(int sim_ticks)
{
	/*
	 * We bias initial_jiffies ahead by one since we will not schedule away
	 * from idle until the first event (specifically, the first timer
	 * tick).
	 */
	u64 initial_jiffies = jiffies;
	cpumask_t runnable;
	int i;

	for_each_online_cpu(i) {
		linsched_change_cpu(i);
		linsched_current_handler();
	}

	simulation_started = true;
	while (current_time < KTIME_MAX
	       && jiffies < initial_jiffies + sim_ticks) {
		u64 evt = KTIME_MAX;
		/* find the next event */
		for (i = 0; i < nr_cpu_ids; i++) {
			if (next_event[i] < evt) {
				evt = next_event[i];
				cpumask_clear(&runnable);
			}
			if (next_event[i] == evt) {
				cpumask_set_cpu(i, &runnable);
			}
		}
		current_time = evt;
		int active_cpu = 0;

		compute_lb_info();

		/* It might be useful to randomize the ordering here, although
		 * it should be rare that there will actually be two active
		 * cpus at once as the tick code offsets the main scheduler
		 * ticks for each cpu */
		for_each_cpu(active_cpu, &runnable) {
			next_event[active_cpu] = KTIME_MAX;
			linsched_change_cpu(active_cpu);
			local_irq_disable();
			irq_enter();
			linsched_hrt[active_cpu].event_handler(&linsched_hrt
							       [active_cpu]);
			irq_exit();
			local_irq_enable();
			/* a handler should never leave this state changed */
			BUG_ON(smp_processor_id() != active_cpu);

			process_all_softirqs();
			linsched_rcu_invoke();

			BUG_ON(irqs_disabled());
			if (idle_cpu(active_cpu) && !need_resched()) {
				tick_nohz_idle_enter();
			} else {
				linsched_current_handler();
			}

			track_nohz_residency(active_cpu);
			run_sanity_check();
		}
	}
}

struct clocksource *__init __weak clocksource_default_clock(void)
{
	return &linsched_hrt_source;
}

static int linsched_hrt_set_next_event(unsigned long evt,
				       struct clock_event_device *d)
{
	next_event[d - linsched_hrt] =
	    ktime_to_ns(ktime_add_safe
			(ns_to_ktime(current_time), ns_to_ktime(evt)));
	return 0;
}

static void linsched_hrt_set_mode(enum clock_event_mode mode,
				  struct clock_event_device *d)
{
}

static void linsched_hrt_broadcast(const struct cpumask *mask)
{
}

static cycle_t linsched_source_read(struct clocksource *cs)
{
	return current_time;
}

u64 sched_clock(void)
{
	return current_time;
}
