#include "linsched.h"
#include <assert.h>

/* for in_sched_functions() */
char __sched_text_start[1];
char __sched_text_end[1];

static struct rcu_head *rcu_head;
static struct rcu_head **rcu_curtail = &rcu_head;
/* 'rcu' */
void call_rcu_sched(struct rcu_head *head, void (*func)(struct rcu_head *rcu))
{
	head->func = func;
	head->next = NULL;

	*rcu_curtail = head;
	rcu_curtail = &head->next;
}

void linsched_rcu_invoke(void)
{
	while(rcu_head) {
		struct rcu_head *next = rcu_head->next;
		rcu_head->func(rcu_head);
		rcu_head = next;
	}
}

/* kernel/workqueue.c */
void wq_worker_waking_up(struct task_struct *task, unsigned int cpu) {}

struct task_struct *wq_worker_sleeping(struct task_struct *task,
				       unsigned int cpu)
{
	/* should be unused */
	assert(0);

	return NULL;
}

/* posix-timers.c */
int __group_send_sig_info(int sig, struct siginfo *info, struct task_struct *p)
{
	/* should be unused */
	assert(0);

	return 0;
}

struct k_itimer;
int posix_timer_event(struct k_itimer *timr, int si_private)
{
	/* should be unused */
	assert(0);

	return 0;
}

struct k_clock;
void posix_timers_register_clock(const clockid_t clock_id,
				 struct k_clock *new_clock)
{
	/* should be unused */
	assert(0);
}

/* kernel/smp.c */
void __smp_call_function_single(int cpu, struct call_single_data *data,
				int wait)
{
	int this_cpu = smp_processor_id();
	unsigned long flags;

	if (cpu != this_cpu)
		linsched_change_cpu(cpu);

	local_irq_save(flags);
	data->func(data->info);
	local_irq_restore(flags);

	if (cpu != this_cpu)
		linsched_change_cpu(this_cpu);
}


void wake_up_stop_task(int cpu, cpu_stop_fn_t fxn, void *data);

/* kernel/stop_machine.c */
void linsched_current_handler(void);

int stop_one_cpu(unsigned int cpu, cpu_stop_fn_t fxn, void *arg)
{
	struct task_struct *stop = stop_tasks[cpu];
	int this_cpu = smp_processor_id(), ret;

	linsched_change_cpu(cpu);
	wake_up_process(stop);
	/* switch to stop */
	schedule();
	BUG_ON(current != stop);
	ret = fxn(arg);
	/* switch back */
	stop->state = TASK_INTERRUPTIBLE;
	schedule();

	/* let whoever followed the stop task re-program (if needed) */
	linsched_current_handler();
	BUG_ON(current == stop);
	linsched_change_cpu(this_cpu);

	return ret;
}

void stop_one_cpu_nowait(unsigned int cpu, cpu_stop_fn_t fxn, void *arg,
			 struct cpu_stop_work *work_buf)
{
	struct task_struct *stop = stop_tasks[cpu];
	struct stop_task *stop_task = task_thread_info(stop)->td->data;
	int this_cpu = smp_processor_id();

	/*
	 * catch a data race on simultaneous calls to stop_one_cpu_nowait, if
	 * this ends up happening we'd need to support a proper queue here.
	 */
	BUG_ON(stop_task->fxn);
	stop_task->fxn = fxn;
	stop_task->arg = arg;

	/*
	 * in the nowait case we actually go through a schedule to make sure
	 * that our cpu has a change to drop its rq->locks since they may be
	 * needed.
	 */
	linsched_change_cpu(cpu);
	hrtimer_set_expires(&stop_task->timer, ns_to_ktime(1));
	hrtimer_start_expires(&stop_task->timer, HRTIMER_MODE_REL);
	linsched_change_cpu(this_cpu);
}

int __stop_machine(int (*fn)(void *), void *data, const struct cpumask *cpus)
{
	/* technically we should bounce every cpu, but don't bother */
	return stop_one_cpu(cpumask_first(cpus), fn, data);
}

int stop_machine(int (*fn)(void *), void *data, const struct cpumask *cpus)
{
	int ret;

	/* No CPUs can come up or down during this. */
	get_online_cpus();
	ret = __stop_machine(fn, data, cpus);
	put_online_cpus();
	return ret;
}

/* the below are unfortunately currently static so we re-define them */
__attribute__((weak)) struct task_group *cgroup_tg(struct cgroup *cgrp)
{
	return container_of(cgroup_subsys_state(cgrp, cpu_cgroup_subsys_id),
			    struct task_group, css);
}

void __sched preempt_schedule(void)
{
};

void fire_sched_out_preempt_notifiers(struct task_struct *curr,
				      struct task_struct *next)
{
}

void fire_sched_in_preempt_notifiers(struct task_struct *curr)
{
}

int security_task_getscheduler(struct task_struct *p,
			       int policy, struct sched_param *lp)
{
	return 0;
}

int security_task_setscheduler(struct task_struct *p,
			       int policy, struct sched_param *lp)
{
	return 0;
}

int cap_task_setnice(struct task_struct *p, int nice)
{
	return 1;
}

int cap_task_setscheduler(struct task_struct *p, int policy,
			struct sched_param *lp)
{
	return 1;
}
