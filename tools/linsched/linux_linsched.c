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
 * the Free Software Foundation; either version 2 of the License, ofalse
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

#include <stdlib.h>

#include <linux/tick.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/percpu_counter.h>
#include <linux/err.h>
#include <linux/dcache.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <dirent.h>
#include <assert.h>

#include "linsched_rand.h"
#include "load_balance_score.h"

/* linsched variables and functions */

/* delaying simulation until run_sim() prevents initial fork balancing. */
bool simulation_started = true;

struct task_struct *__linsched_tasks[LINSCHED_MAX_TASKS];
int curr_task_id;

struct linsched_cgroup __linsched_cgroups[LINSCHED_MAX_GROUPS];
int num_cgroups = 1, num_tasks = 1;
extern int max_threads;
int oops_in_progress;

asmlinkage void __init start_kernel(void);
struct page linsched_page;
enum system_states system_state;
char __lock_text_start[] = ".", __lock_text_end[] = ".";
struct cgroup *root_cgroup = &__linsched_cgroups[0].cg;

void linsched_init_root_cgroup(struct cgroup *root)
{
	char *root_path = "/";

	root->dentry = malloc(sizeof(struct dentry));
	root->dentry->d_name.name = strdup(root_path);
	root->dentry->d_name.len = strlen(root_path);

	root_cgroup->subsys[cpuacct_subsys_id] = cpuacct_subsys.create(NULL,
								root_cgroup);
}

void init_stop_tasks(void);

void linsched_init(struct linsched_topology *topo)
{
	curr_task_id = 1;
	linsched_init_cpus(topo);

	/* Change context to "boot" cpu and boot kernel. */
	linsched_change_cpu(0);

	sched_clock_stable = 1;

	/*TODO: set an appropriate limit*/
	max_threads = INT_MAX;

	system_state = SYSTEM_BOOTING;
	start_kernel();
	system_state = SYSTEM_RUNNING;

	linsched_init_root_cgroup(root_cgroup);
	linsched_init_hrtimer();

	init_lb_info();
	init_stop_tasks();
}

void linsched_default_callback(void)
{
}

void linsched_exit_callback(void)
{
	do_exit(0);
}

struct cfs_rq;
void linsched_announce_callback(void)
{
	printf("CPU %d / t = %u: Task 0x%d scheduled, Runtime %llu\n",
	       smp_processor_id(), (unsigned int)jiffies,
	       task_thread_info(current)->id,
	       current->se.sum_exec_runtime);
}

void linsched_disable_migrations(void)
{
	int i;

	for (i = 1; i < num_tasks; i++)
		set_cpus_allowed(__linsched_tasks[i],
				 cpumask_of_cpu(task_cpu(__linsched_tasks[i])));
}

void linsched_enable_migrations(void)
{
	int i;

	for (i = 1; i < num_tasks; i++)
		set_cpus_allowed(__linsched_tasks[i], CPU_MASK_ALL);
}

/* Force a migration of task to the dest_cpu.
 * If migr is set, allow migrations after the forced migration... otherwise,
 * do not allow them. (We need to disable migrations so that the forced
 * migration takes place correctly.)
 * Returns old cpu of task.
 */
int linsched_force_migration(struct task_struct *task, int dest_cpu, int migr)
{
	int old_cpu = task_cpu(task);

	linsched_disable_migrations();
	set_cpus_allowed(task, cpumask_of_cpu(dest_cpu));
	linsched_change_cpu(old_cpu);
	schedule();
	linsched_change_cpu(dest_cpu);
	schedule();
	scheduler_ipi();
	if (migr)
		linsched_enable_migrations();

	return old_cpu;
}

/* Return the task in position task_id in the task array.
 * No error checking, so be careful!
 */
struct task_struct *linsched_get_task(int task_id)
{
	return __linsched_tasks[task_id];
}

static struct task_struct *linsched_fork(struct task_data *td, int enqueue_start)
{
	struct task_struct *p;

	//p = linsched_raw_copy_process();
	p = find_task_by_vpid(do_fork(0, 0, NULL, 0, NULL, NULL));

	task_thread_info(p)->td = td;
	/*
	 * This is fine *only* because we don't care about mm
	 * TODO: Might have to fix this assumption
	 */
	p->mm = &init_mm;
	p->active_mm = &init_mm;

	if (!enqueue_start) {
		struct rq *rq = cpu_rq(task_cpu(p));

		raw_spin_lock_irq(&rq->lock);
		deactivate_task(rq, p, DEQUEUE_SLEEP);
		p->on_rq = 0;
		raw_spin_unlock_irq(&rq->lock);
	}

	return p;
}

static struct task_struct *__linsched_create_task(struct task_data *td)
{
	struct task_struct *newtask = linsched_fork(td, true);

	if (td->init_task)
		td->init_task(newtask, td->data);

	/* allow task to run on any cpu. */
	set_cpus_allowed(newtask, CPU_MASK_ALL);

	linsched_check_resched();

	return newtask;
}

extern void linsched_check_idle_cpu(void);
void linsched_check_resched(void)
{
	if (!simulation_started)
		return;

	while (need_resched()) {
		/*
		 * Check if we need to exit nohz mode.
		 */
		linsched_check_idle_cpu();
		schedule();
	}
}

void __linsched_set_task_id(struct task_struct *p, int id)
{
	task_thread_info(p)->id = id;
	sprintf(p->comm, "%d", id);
}

const char *cgroup_name(struct cgroup *cgrp)
{
	return cgrp->dentry->d_name.name;
}

struct cgroup *linsched_create_cgroup(struct cgroup *parent, char *path)
{
	struct cgroup *child;
	int id = num_cgroups++;
	char buf[64];
	child = &__linsched_cgroups[id].cg;

	assert(num_cgroups < LINSCHED_MAX_GROUPS);
	/* we have to set up the parent pointer before creating sub-systems */
	child->parent = parent;
	child->subsys[cpu_cgroup_subsys_id] = cpu_cgroup_subsys.create(NULL,
								child);
	child->subsys[cpu_cgroup_subsys_id]->cgroup = child;
	child->subsys[cpuacct_subsys_id] = cpuacct_subsys.create(NULL,
								child);
	child->subsys[cpuacct_subsys_id]->cgroup = child;

	if (!path) {
		sprintf(buf, "cg%d", id);
		path = buf;
	}

	child->dentry = malloc(sizeof(struct dentry));
	child->dentry->d_name.name = strdup(path);
	child->dentry->d_name.len = strlen(path);

	return child;
}

int linsched_add_task_to_group(struct task_struct *p, struct cgroup *cgrp)
{
	struct task_group *tg = cgroup_tg(cgrp);
	struct cpuacct *ca = cgroup_ca(cgrp);

	p->cgroups->subsys[cpu_cgroup_subsys_id] = &tg->css;
	p->cgroups->subsys[cpuacct_subsys_id] = &ca->css;
	sched_move_task(p);

	return 0;
}

u64 task_exec_time(struct task_struct *p)
{
	return p->se.sum_exec_runtime;
}

u64 group_exec_time(struct task_group *tg)
{
	int i;
	u64 total = 0;

	/* this depends on schedstats, but works for the root cgroup */
	for_each_possible_cpu(i)
		total += tg->cfs_rq[i]->exec_clock;

	return total;
}

int show_schedstat(struct seq_file *seq, void *v);

int linsched_show_schedstat(void)
{
	return show_schedstat(NULL, NULL);
}

void linsched_print_task_stats(void)
{
	int i;
	long total_time = 0;
	for (i = 1; i < num_tasks; i++) {
		struct task_struct *task = __linsched_tasks[i];
		printf
		    ("Task id = %d (%d), exec_time = %llu, run_delay = %llu, pcount = %lu\n",
		     task_pid_nr(task), i, task_exec_time(task), task->sched_info.run_delay,
		     task->sched_info.pcount);

		total_time += task_exec_time(task);
	}
	printf("Total exec_time = %ld\n", total_time);
}

/* We could have used the cpuacct cgroup for this, but a lot of the code
 * would have to be compiled out leading to a bunch of #ifdefs all over
 * the place. The simple function below is all we need.
 */
void linsched_print_group_stats(void)
{
	char buf[128];
	int i;
	for (i = 0; i < num_cgroups; i++) {
		struct cgroup *cgrp = &__linsched_cgroups[i].cg;
		cgroup_path(cgrp, buf, 128);
		printf("CGroup = %s (%d), exec_time = %llu\n",
				buf, i, group_exec_time(cgroup_tg(cgrp)));
	}
}

/* Create a normal task with the specified callback and
 * nice value of niceval, which determines its priority.
 */
struct task_struct *linsched_create_normal_task(struct task_data *td, int niceval)
{
	struct sched_param params = {};
	struct task_struct *p;
	int id = num_tasks++;

	assert(id < LINSCHED_MAX_TASKS);

	/* Create "normal" task and set its nice value. */
	p = __linsched_tasks[id] = __linsched_create_task(td);
	__linsched_set_task_id(p, id);

	params.sched_priority = 0;
	sched_setscheduler(p, SCHED_NORMAL, &params);
	set_user_nice(p, niceval);

	assert(current->cgroups->subsys[0]);
	assert(p->cgroups->subsys[0]);
	return p;
}

/* Create a batch task with the specified callback and
 * nice value of niceval, which determines its priority.
 */
struct task_struct *linsched_create_batch_task(struct task_data *td, int niceval)
{
	struct sched_param params = { };
	int id = num_tasks++;
	struct task_struct *p;

	assert(id < LINSCHED_MAX_TASKS);

	/* Create "batch" task and set its nice value. */
	p = __linsched_tasks[id] = __linsched_create_task(td);
	__linsched_set_task_id(p, id);

	params.sched_priority = 0;
	sched_setscheduler(p, SCHED_BATCH, &params);
	set_user_nice(p, niceval);

	return p;
}

/* Create a FIFO real-time task with the specified callback and priority. */
struct task_struct *linsched_create_RTfifo_task(struct task_data *td, int prio)
{
	struct sched_param params = { };
	int id = num_tasks++;
	struct task_struct *p;

	assert(id < LINSCHED_MAX_TASKS);

	/* Create FIFO real-time task and set its priority. */
	p = __linsched_tasks[id] = __linsched_create_task(td);
	__linsched_set_task_id(p, id);

	params.sched_priority = prio;
	sched_setscheduler(p, SCHED_FIFO, &params);

	return p;
}

/* Create a RR real-time task with the specified callback and priority. */
struct task_struct *linsched_create_RTrr_task(struct task_data *td, int prio)
{
	struct sched_param params = { };
	int id = num_tasks++;
	struct task_struct *p;

	assert(id < LINSCHED_MAX_TASKS);

	/* Create RR real-time task and set its priority. */
	p = __linsched_tasks[id] = __linsched_create_task(td);

	params.sched_priority = prio;
	sched_setscheduler(p, SCHED_RR, &params);

	return p;
}

void linsched_yield(void)
{
	/* If the current task is not the idle task, yield. */
	if (current != idle_task(smp_processor_id()))
		yield();
}

u64 getticks(void)
{
	u64 a, d;
	asm volatile("lfence; rdtsc" : "=a" (a), "=d" (d));

	return (a | (d << 32));
}

u64 rq_clock_delta(struct task_struct *p);
/* The standard task_data for a task which runs the busy/sleep setup */
static enum hrtimer_restart wake_task(struct hrtimer *timer)
{
	struct sleep_run_data *d =
	    container_of(timer, struct sleep_run_data, timer);
	wake_up_process(d->p);
	return HRTIMER_NORESTART;
}

static void sleep_run_init(struct sleep_run_data *d)
{
	d->last_start = 0;
	hrtimer_init(&d->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	d->timer.function = wake_task;
}

static void sleep_run_start(struct task_struct *p, void *data)
{
	struct sleep_run_data *d = data;
	d->p = p;
}

/* Go to sleep in the given state and schedule a wakeup in ns nanoseconds */
static void sleep_run_sleep_for(struct sleep_run_data *d, int state, u64 ns)
{
	hrtimer_set_expires(&d->timer, ns_to_ktime(ns));
	hrtimer_start_expires(&d->timer, HRTIMER_MODE_REL);

	if(d->p) {
		d->p->state = state;
		schedule();
	}
}

/* Start or continue a run for ns nanoseconds. Tries to schedule a
 * timer to end the run at the exact correct time. Returns true when
 * the run has been finished. */
static int sleep_run_run_for(struct sleep_run_data *d, u64 ns)
{
	struct task_struct *p = d->p;
	/*
	 * start at time 1 so that we can reserve 0 for "just started
	 * this run"
	 */
	s64 delta = 0, runtime = 1;

	if(p) {
		runtime += task_exec_time(p);
		if(p->on_rq) {
			/* this isn't quite the "right" clock, but is the best
			 * we can do without calling into the scheduler to
			 * update rq->clock_task (which should be the same
			 * anyway in linsched land) */
			runtime += current_time - p->se.exec_start;
		}
	}

	if (!d->last_start) {
		d->last_start = runtime;
	} else {
		delta = runtime - d->last_start;
	}

	if (delta >= ns) {
		d->last_start = 0;
		return 1;
	}

	hrtimer_set_expires(&d->timer, ns_to_ktime(ns - delta));
	hrtimer_start_expires(&d->timer, HRTIMER_MODE_REL);
	return 0;
}

/* @fp: valid ptr to rlog file
 * extracts the next event from the rlog file*/
struct linsched_perf_event perf_get_next_event(FILE *fp)
{
	struct linsched_perf_event pe = {};
	char *delim = ",";
	char *event_type = NULL, *duration = NULL;
	char line[256];

	if (fp && !feof(fp)) {
		if (fgets(line, sizeof(line), fp)) {
			strtok(line, delim);
			event_type = strtok(NULL, delim);
			strtok(NULL, delim);
			duration = strtok(NULL, delim);
		}
		if (event_type && duration) {
			pe.duration = simple_strtol(duration, NULL, 0);
			if (!strcasecmp("R", event_type))
				pe.type = RUN;
			else if (!strcasecmp("S", event_type))
				pe.type = SLEEP;
			else
				pe.type = IOWAIT;
		}
	}
	return pe;
}

static void sleep_run_handle(struct task_struct *p, void *data)
{
	struct sleep_run_task *d = data;
	if (sleep_run_run_for(&d->sr_data, d->busy * NSEC_PER_MSEC)) {
		if(d->sleep) {
			sleep_run_sleep_for(&d->sr_data, TASK_INTERRUPTIBLE,
					    d->sleep * NSEC_PER_MSEC);
		} else {
			sleep_run_run_for(&d->sr_data, d->busy * NSEC_PER_MSEC
					 );
		}
	}
}

struct task_data *linsched_create_sleep_run(int sleep, int busy)
{
	struct task_data *td = malloc(sizeof(struct task_data));
	struct sleep_run_task *d = malloc(sizeof(struct sleep_run_task));
	d->sleep = sleep;
	d->busy = busy;
	sleep_run_init(&d->sr_data);
	td->data = d;
	td->init_task = sleep_run_start;
	td->handle_task = sleep_run_handle;
	return td;
}

void rcu_note_context_switch(int cpu) {}

struct task_struct *find_task_by_vpid(pid_t vnr);
/*struct task_struct *find_task_by_vpid(pid_t vnr)
{
	struct task_struct *p;

	if (vnr >= LINSCHED_MAX_TASKS)
		return NULL;

	p = __linsched_tasks[vnr];
	BUG_ON(p->pid != vnr);

	return p;
}*/

/* get the next sleep / busy values from the appropriate distribution */
static void rnd_dist_sleep_run_handle(struct task_struct *p, void *data)
{
	struct rnd_dist_task *d = data;
	struct rand_dist *sdist = d->sleep_rdist;
	struct rand_dist *bdist = d->busy_rdist;

	if (sleep_run_run_for(&d->sr_data, d->busy)) {
		d->sleep = sdist->gen_fn(sdist);
		d->busy = bdist->gen_fn(bdist);

		/* in case sleep is 0, we should not lose our task */
		if (!d->sleep)
			d->sleep = 1;
		sleep_run_sleep_for(&d->sr_data, TASK_INTERRUPTIBLE, d->sleep);
	}
}

struct task_data *linsched_create_rnd_dist_sleep_run(struct rand_dist
						     *sleep_rdist, struct rand_dist
						     *busy_rdist)
{
	struct task_data *td = malloc(sizeof(struct task_data));
	struct rnd_dist_task *d = malloc(sizeof(struct rnd_dist_task));

	d->sleep_rdist = sleep_rdist;
	d->busy_rdist = busy_rdist;

	d->sleep = sleep_rdist->gen_fn(sleep_rdist);
	d->busy = busy_rdist->gen_fn(busy_rdist);
	sleep_run_init(&d->sr_data);

	td->data = d;
	td->init_task = sleep_run_start;
	td->handle_task = rnd_dist_sleep_run_handle;
	return td;
}

static void perf_task_handle(struct task_struct *p, void *data)
{
	struct perf_task *d = data;
	if (sleep_run_run_for(&d->sr_data, d->busy)) {
		struct linsched_perf_event pe = perf_get_next_event(d->fp);
		if (pe.duration) {
			if (pe.type == RUN) {
				d->busy = pe.duration;
				d->sleep = 0;
				sleep_run_run_for(&d->sr_data, d->busy);
			} else {
				d->sleep = pe.duration;
				d->busy = 0;
			}
		} else {
			/* didn't find an event, sleep */
			d->sleep = UINT_MAX;
			d->busy = 0;
		}
		if (d->sleep) {
			int state;
			if (pe.type == SLEEP)
				state = TASK_INTERRUPTIBLE;	/* sleep */
			else
				state = TASK_UNINTERRUPTIBLE;  /* iowait */
			sleep_run_sleep_for(&d->sr_data, state, d->sleep);
		}
	}
}

struct task_data *linsched_create_perf_task(char *filename)
{
	struct task_data *td = malloc(sizeof(struct task_data));
	struct perf_task *d =  malloc(sizeof(struct perf_task));

	memset(d, 0, sizeof(*d));
	sleep_run_init(&d->sr_data);

	d->fp = fopen(filename, "r");
	if (d->fp) {
		d->filename = filename;
		perf_task_handle(NULL, d);
	} else {
		fprintf(stderr, "\nopening %s failed. Error : %s",
				filename, strerror(errno));
		free(td);
		free(d);
		return NULL;
	}

	td->data = d;
	td->init_task = sleep_run_start;
	td->handle_task = perf_task_handle;
	return td;
}

/* creates perf tasks for rlogs in directory
 * @_dirpath: path to directory
 * returns 0 on success, negative on failure
 */
int linsched_create_perf_tasks(char *_dirpath)
{
	DIR *dir;
	struct dirent *de;
	struct task_data *td;
	char *dirpath = malloc(strlen(_dirpath) + 1);
	char *filepath = malloc(strlen(dirpath) + NAME_MAX);

	strcpy(dirpath, _dirpath);

	/* check if directory name appended with '/' */
	if (_dirpath[strlen(_dirpath) - 1] != '/')
		strcat(dirpath, "/");

	dir = opendir(dirpath);

	if (dir) {
		while ((de = readdir(dir))) {
			filepath = strcpy(filepath, dirpath);
			strcat(filepath, de->d_name);
			if (strstr(de->d_name, ".rlog")) {
				td = linsched_create_perf_task(filepath);
				if (td)
					linsched_create_normal_task(td, 0);
			}
		}
		closedir(dir);
	} else {
		fprintf(stderr, "\nopening %s directory failed.", dirpath);
		return -1;
	}

	free(filepath);
	free(dirpath);
	return 0;
}

struct task_struct *stop_tasks[NR_CPUS];

static void stop_task_handle(struct task_struct *p, void *data)
{
	struct stop_task *stop_task = data;

	if (stop_task->fxn)
		stop_task->fxn(stop_task->arg);
	else
		BUG();

	stop_task->fxn = NULL;
	p->state = TASK_INTERRUPTIBLE;
	schedule();
}

void sched_set_stop_task(int cpu, struct task_struct *stop);

static enum hrtimer_restart wake_stop(struct hrtimer *timer)
{
	struct stop_task *d = container_of(timer, struct stop_task, timer);
	wake_up_process(d->p);
	return HRTIMER_NORESTART;
}

struct task_struct *linsched_create_stop_task(int cpu)
{
	struct task_struct *p;
	struct task_data *td = malloc(sizeof(struct task_data));
	struct stop_task *d = malloc(sizeof(struct stop_task));
	cpumask_t mask;

	memset(d, 0, sizeof(struct stop_task));
	td->data = d;
	td->handle_task = stop_task_handle;
	hrtimer_init(&d->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	p = linsched_fork(td, false);
	d->timer.function = wake_stop;
	d->p = p;
	cpumask_clear(&mask);
	cpumask_set_cpu(cpu, &mask);
	set_cpus_allowed(p, mask);
	sched_set_stop_task(cpu, p);
	set_task_cpu(p, cpu);
	p->state = TASK_INTERRUPTIBLE;

	return p;
}

void init_stop_tasks(void)
{
	int cpu;

	for_each_possible_cpu(cpu)
		stop_tasks[cpu] = linsched_create_stop_task(cpu);
}

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
	return strtoul(cp, endp, base);
}

long simple_strtol(const char *cp, char **endp, unsigned int base)
{
	return strtol(cp, endp, base);
}

int scnprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buf, size, fmt, args);
	va_end(args);

	return i;
}
