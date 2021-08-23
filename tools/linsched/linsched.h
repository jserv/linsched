/* LinSched -- The Linux Scheduler Simulator
 * Copyright (C) 2008  John M. Calandrino
 * E-mail: jmc@cs.unc.edu
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

#ifndef LINSCHED_H
#define LINSCHED_H

#include "linux_sched_headers.h"

/* Linsched definitions and declarations. */
#define LINSCHED_RAND_SEED	123456
#define LINSCHED_MAX_TASKS	10000
#define LINSCHED_MAX_GROUPS	100
#define LINSCHED_DEFAULT_NR_CPUS 4

/* have we started the simulation */
extern bool simulation_started;

extern struct cgroup *root_cgroup;

extern struct task_struct *__linsched_tasks[LINSCHED_MAX_TASKS];
extern struct task_struct *stop_tasks[NR_CPUS];

struct linsched_cgroup {
	struct cgroup cg;
	void *temp;
};

extern struct linsched_cgroup __linsched_cgroups[LINSCHED_MAX_GROUPS];

extern u64 current_time;

extern cpumask_t linsched_cpu_resched_pending;

struct task_data {
	void *data;
	void (*init_task) (struct task_struct *, void *data);
	void (*handle_task) (struct task_struct *, void *data);
};

struct stop_task {
	cpu_stop_fn_t fxn;
	void *arg;

	struct task_struct *p;
	struct hrtimer timer;
};

struct sleep_run_data {
	struct hrtimer timer;
	struct task_struct *p;
	u64 last_start;
};

struct sleep_run_task {
	struct sleep_run_data sr_data;
	unsigned int busy;	/* ms to run */
	unsigned int sleep;	/* ms to sleep */
};

/* A generic data placeholder for tasks modelled using random dists.
 */
struct rnd_dist_task {
	struct sleep_run_data sr_data;
	unsigned int busy;
	unsigned int sleep;
	struct rand_dist *sleep_rdist, *busy_rdist;
};

struct perf_task {
	struct sleep_run_data sr_data;
	unsigned int busy;	/* ms to run */
	unsigned int sleep;	/* ms to sleep */
	char *filename;		/* pid.rlog file */
	void *fp;		/* fp to pid.rlog file */
};

enum linsched_perf_event_type {
	RUN,
	SLEEP,
	IOWAIT,
};

struct linsched_perf_event {
	enum linsched_perf_event_type type;
	unsigned long duration;
};

/* Used to specify the topology of the system. Not specifying a
 * topology gives a flat topology of LINSCHED_DEFAULT_NR_CPUS CPUs,
 * each with one core and no SMT */
struct linsched_topology {
	/* map from logical cpu to containing NUMA node */
	int node_map[NR_CPUS];
	/* map to containing physical CPU package */
	int coregroup_map[NR_CPUS];
	/* map from SMT logical cpu to containing core */
	int core_map[NR_CPUS];
	/* map from [node][node] to distance between them */
	int node_distances[MAX_NUMNODES][MAX_NUMNODES];
	int nr_cpus;
};

enum topologies {
	UNIPROCESSOR,
	DUAL_CPU,
	DUAL_CPU_MC,
	QUAD_CPU,
	QUAD_CPU_MC,
	QUAD_CPU_DUAL_SOCKET,
	QUAD_CPU_QUAD_SOCKET,
	HEX_CPU_DUAL_SOCKET_SMT,
	MAX_TOPOLOGIES
};

#define TOPO_UNIPROCESSOR					\
	{							\
		.nr_cpus = 1,					\
		.core_map = {					\
			[0] = 0,				\
		},						\
		.coregroup_map = {				\
			[0] = 0,				\
		},						\
		.node_map = {					\
			[0] = 0,				\
		},						\
		.node_distances = {				\
			{ 10 },					\
		},						\
	}

#define TOPO_DUAL_CPU						\
	{							\
		.nr_cpus = 2,					\
		.core_map = {					\
			[0] = 0,				\
			[1] = 1,				\
		},						\
		.coregroup_map = {				\
			[0] = 0,				\
			[1] = 1,				\
		},						\
		.node_map = {					\
			[0 ... 1] = 0,				\
		},						\
		.node_distances = {				\
			{ 10 },					\
		},						\
	}

#define TOPO_DUAL_CPU_MC					\
	{							\
		.nr_cpus = 2,					\
		.core_map = {					\
			[0] = 0,				\
			[1] = 1,				\
		},						\
		.coregroup_map = {				\
			[0 ... 1] = 0,				\
		},						\
		.node_map = {					\
			[0 ... 1] = 0,				\
		},						\
		.node_distances = {				\
			{ 10 },					\
		},						\
	}

#define TOPO_QUAD_CPU						\
	{							\
		.nr_cpus = 4,					\
		.core_map = {					\
			0, 1, 2, 3,				\
		},						\
		.coregroup_map = {				\
			0, 1, 2, 3,				\
		},						\
		.node_map = {					\
			[0 ... 3] = 0,				\
		},						\
		.node_distances = {				\
			{ 10 },					\
		},						\
	}

#define TOPO_QUAD_CPU_MC					\
	{							\
		.nr_cpus = 4,					\
		.core_map = {					\
			0, 1, 2, 3,				\
		},						\
		.coregroup_map = {				\
			[0 ... 1] = 0,				\
			[2 ... 3] = 1,				\
		},						\
		.node_map = {					\
			[0 ... 3] = 0,				\
		},						\
		.node_distances = {				\
			{ 10 },					\
		},						\
	}

#define TOPO_QUAD_CPU_DUAL_SOCKET				\
	{							\
		.nr_cpus = 8,					\
		.core_map = {					\
			0, 1, 2, 3,				\
			4, 5, 6, 7,				\
		},						\
		.coregroup_map = {				\
			[0] = 0,				\
			[1] = 1,				\
			[2] = 2,				\
			[3] = 3,				\
			[4] = 0,				\
			[5] = 1,				\
			[6] = 2,				\
			[7] = 3,				\
		},						\
		.node_map = {					\
			[0 ... 7] = 0,				\
		},						\
		.node_distances = {				\
			{ 10 },					\
		},						\
	}

#define TOPO_QUAD_CPU_QUAD_SOCKET				\
	{							\
		.nr_cpus = 16,					\
		.core_map = {					\
			0, 1, 2, 3,				\
			4, 5, 6, 7,				\
			8, 9, 10, 11,				\
			12, 13, 14, 15,				\
		},						\
		.coregroup_map = {				\
			[0 ... 3] = 0,				\
			[4 ... 7] = 1,				\
			[8 ... 11] = 2,				\
			[12 ... 15] = 3,			\
		},						\
		.node_map = {					\
			[0 ... 3] = 0,				\
			[4 ... 7] = 1,				\
			[8 ... 11] = 2,				\
			[12 ... 15] = 3,			\
		},						\
		.node_distances = {				\
			{ 10, 20, 20, 30 },			\
			{ 20, 20, 10, 20 },			\
			{ 20, 10, 20, 20 },			\
			{ 30, 20, 20, 10 },			\
		},						\
	}

#define TOPO_HEX_CPU_DUAL_SOCKET_SMT				\
	{							\
		.nr_cpus = 24,					\
		.core_map = {					\
			[0] = 0,				\
			[1] = 1,				\
			[2] = 2,				\
			[3] = 3,				\
			[4] = 4,				\
			[5] = 5,				\
			[6] = 6,				\
			[7] = 7,				\
			[8] = 8,				\
			[9] = 9,				\
			[10] = 10,				\
			[11] = 11,				\
			[12] = 0,				\
			[13] = 1,				\
			[14] = 2,				\
			[15] = 3,				\
			[16] = 4,				\
			[17] = 5,				\
			[18] = 6,				\
			[19] = 7,				\
			[20] = 8,				\
			[21] = 9,				\
			[22] = 10,				\
			[23] = 11,				\
		},						\
		.coregroup_map = {				\
			[0 ... 5] = 0,				\
			[6 ... 11] = 1,				\
			[12 ... 17] = 0,			\
			[18 ... 23] = 1,			\
		},						\
		.node_map = {					\
			[0 ... 5] = 0,				\
			[6 ... 11] = 1,				\
			[12 ... 17] = 0,			\
			[18 ... 23] = 1,			\
		},						\
		.node_distances = {				\
			{ 10, 20 },				\
			{ 20, 10 },				\
		},						\
	}

struct linsched_global_options {
	int print_nohz;
	int print_tasks;
	int print_cgroups;
	int print_avg_imb;
	int print_sched_stats;
	int dump_imbalance;
	int dump_full_balance;
};
extern struct linsched_global_options linsched_global_options;

/* Declarations of system initialization (or "boot") function. */
asmlinkage void __init start_kernel(void);

/* Scheduler "system calls" that are used during task creation. */
asmlinkage long sys_sched_setscheduler(pid_t pid, int policy,
				       struct sched_param __user *param);
asmlinkage long sys_nice(int increment);

/* sched.c functions */
struct task_group *cgroup_tg(struct cgroup *cgrp);
struct cpuacct *cgroup_ca(struct cgroup *cgrp);
struct cpuacct *task_ca(struct task_struct *tsk);
struct task_group *task_group(struct task_struct *p);

/* cgroup functions */
const char *cgroup_name(struct cgroup *cgrp);
struct cgroup *linsched_create_cgroup(struct cgroup *parent, char *path);
struct task_group *cgroup_tg(struct cgroup *cgrp); /* from sched.c */
struct cpuacct *cgroup_ca(struct cgroup *cgrp); /* from sched.c */
struct cpuacct *task_ca(struct task_struct *tsk); /* from sched.c */

static inline struct linsched_cgroup *linsched_cgroup(struct cgroup *cg) {
	return container_of(cg, struct linsched_cgroup, cg);
}

static inline struct linsched_cgroup *linsched_tg(struct task_group *tg) {
	return container_of(tg->css.cgroup, struct linsched_cgroup, cg);
}

/* linsched functions */

struct task_struct *linsched_get_task(int task_id);
struct task_data *linsched_create_sleep_run(int sleep, int busy);
struct task_struct *linsched_create_batch_task(struct task_data *, int niceval);
struct task_struct *linsched_create_RTfifo_task(struct task_data *, int prio);
struct task_struct *linsched_create_RTrr_task(struct task_data *, int prio);
struct task_struct *linsched_create_normal_task(struct task_data *td, int niceval);

struct task_struct *linsched_raw_copy_process(void);
int linsched_show_schedstat(void);
void linsched_change_cpu(int cpu);
void linsched_trigger_cpu(int cpu);
void linsched_check_resched(void);
void linsched_init_cpus(struct linsched_topology *topo);
void linsched_init_hrtimer(void);
void linsched_init(struct linsched_topology *topo);
void linsched_default_callback(void);
void linsched_announce_callback(void);
void linsched_exit_callback(void);
void linsched_disable_migrations(void);
void linsched_enable_migrations(void);
int linsched_force_migration(struct task_struct *task, int dest_cpu, int migr);
int linsched_create_task_group(int parent);
int linsched_add_task_to_group(struct task_struct *p, struct cgroup *cgrp);
void linsched_set_task_group_shares(int groupid, unsigned long shares);
void linsched_yield(void);
void linsched_random_init(int seed);
unsigned long linsched_random(void);
void linsched_run_sim(int sim_ticks);
void linsched_sched_debug_show(void);
void linsched_print_task_stats(void);
void linsched_print_group_stats(void);

void linsched_print_cpuacct_stats(int cpuacct_group_id);
void linsched_runtime_timer(struct task_struct *t, u64 runtime, void (*fn)(struct task_data *));
void linsched_idled_set(int cpu, long idle_time, long busy_time);
struct task_data *linsched_create_rnd_dist_sleep_run(struct rand_dist *sleep_rdist,
                                                     struct rand_dist *busy_rdist);
u64 getticks(void);
int linsched_create_perf_tasks(char *dirpath);

void linsched_set_printk_level(int level);

void linsched_rcu_invoke(void);

void linsched_online_cpu(int cpu);
void linsched_offline_cpu(int cpu);

#endif /* LINSCHED_H */
