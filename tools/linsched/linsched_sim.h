/* Copyright 2011 Google Inc. All Rights Reserved.
 Author: asr@google.com (Abhishek Srivastava) */

#ifndef __LINSCHED_SIM_H
#define __LINSCHED_SIM_H

#include "linsched.h"

#define MIN_N_TASKS 10
#define MAX_N_TASKS 20

#define MIN_GAUSSIAN_DIST_MEAN 600000
#define MAX_GAUSSIAN_DIST_MEAN 800000
#define MIN_GAUSSIAN_DIST_SD 880000
#define MAX_GAUSSIAN_DIST_SD 900000

#define MIN_POISSON_DIST_MEAN 600000
#define MAX_POISSON_DIST_MEAN 800000

#define MIN_EXPONENTIAL_DIST_MEAN 600000
#define MAX_EXPONENTIAL_DIST_MEAN 700000

#define MIN_LOGNORMAL_DIST_MEANLOG 11
#define MAX_LOGNORMAL_DIST_MEANLOG 14
#define MIN_LOGNORMAL_DIST_SDLOG 1
#define MAX_LOGNORMAL_DIST_SDLOG 4

#define TOPO_ARGO "argo"
#define TOPO_IKARIA "ikaria"

struct linsched_tg_sim {
	int n_tasks;
	struct cgroup *cg;
	struct task_struct **tasks;
};

struct linsched_sim {
	int n_task_grps;
	struct linsched_tg_sim **tg_sim_arr;
};

struct linsched_tg_sim *linsched_create_tg_sim(int shares,
					       struct rand_dist *sleep_dist,
					       struct rand_dist *busy_dist,
					       unsigned int *rand_state,
					       struct cgroup *group,
					       const struct cpumask *cpus);
struct linsched_sim *linsched_create_sim(char *tg_file, const struct cpumask *cpus,
					 unsigned int *rand_state);
void linsched_destroy_rnd_sim_task(struct task_struct *p);
void linsched_destroy_tg_sim(struct linsched_tg_sim *tgsim);
void linsched_destroy_sim(struct linsched_sim *lsim);
void print_report(struct linsched_sim *lsim);

#endif	/* __LINSCHED_SIM_H */
