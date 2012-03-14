#include "linsched.h"
#include "linsched_rand.h"
#include "linsched_sim.h"
#include "load_balance_score.h"
#include <stdio.h>
#include <malloc.h>
#include <assert.h>

/* picks a random dist type */
enum RND_TYPE pick_random_dist_type(unsigned int *rand_state)
{
	enum RND_TYPE type = linsched_rand_range(0, MAX_RND_TYPE,
						rand_state);
	return type;
}

/* returns a gaussian dist with random parameters */
struct rand_dist *pick_gaussian_dist(unsigned int *rand_state)
{
	struct rand_dist *rdist;
	double mean = linsched_rand_range(MIN_GAUSSIAN_DIST_MEAN,
					MAX_GAUSSIAN_DIST_MEAN,
					rand_state);
	double sd = linsched_rand_range(MIN_GAUSSIAN_DIST_SD,
					MAX_GAUSSIAN_DIST_SD,
					rand_state);

	rdist = linsched_init_gaussian(mean, sd, *rand_state);
	return rdist;
}

/* returns a poisson dist with random parameters */
struct rand_dist *pick_poisson_dist(unsigned int *rand_state)
{
	struct rand_dist *rdist;
	double mean = linsched_rand_range(MIN_POISSON_DIST_MEAN,
					MAX_POISSON_DIST_MEAN,
					rand_state);

	rdist = linsched_init_poisson(mean, *rand_state);
	return rdist;
}


/* returns a exponential dist with random parameters */
struct rand_dist *pick_exponential_dist(unsigned int *rand_state)
{
	struct rand_dist *rdist;
	double mean = linsched_rand_range(MIN_EXPONENTIAL_DIST_MEAN,
					MAX_EXPONENTIAL_DIST_MEAN,
					rand_state);

	rdist = linsched_init_exponential(mean, *rand_state);
	return rdist;
}

/* returns a lognormal dist with random parameters */
struct rand_dist *pick_lognormal_dist(unsigned int *rand_state)
{
	struct rand_dist *rdist;
	double meanlog = linsched_rand_range(MIN_LOGNORMAL_DIST_MEANLOG,
					MAX_LOGNORMAL_DIST_MEANLOG,
					rand_state);
	double sdlog = linsched_rand_range(MIN_LOGNORMAL_DIST_SDLOG,
					MAX_LOGNORMAL_DIST_SDLOG,
					rand_state);

	rdist = linsched_init_lognormal(meanlog, sdlog, *rand_state);
	return rdist;
}

/* use LOGNORMAL. */
struct rand_dist *pick_random_run_dist(unsigned int *rand_state)
{
	return pick_lognormal_dist(rand_state);
}

/* use either EXPONENTIAL or LOGNORMAL */
struct rand_dist *pick_random_sleep_dist(unsigned int *rand_state)
{
	enum RND_TYPE type = pick_random_dist_type(rand_state);

	while (type != EXPONENTIAL && type != LOGNORMAL)
		type = pick_random_dist_type(rand_state);
	if (type == EXPONENTIAL)
		return pick_exponential_dist(rand_state);
	else
		return pick_lognormal_dist(rand_state);
}

int pick_n_tasks(unsigned int *rand_state)
{
	int n_tasks = linsched_rand_range(MIN_N_TASKS,
			MAX_N_TASKS, rand_state);
	return n_tasks;
}

/* creates a task group (with the specified shares) and a random
 * number of tasks having random run / sleep distributions. If
 * sleep_type and busy_type are specified, the tasks will have
 * distributions of those types, if they are MAX_RND_TYPE then they
 * are picked randomly */
struct linsched_tg_sim *linsched_create_tg_sim(int shares,
					       struct rand_dist *sleep_dist,
					       struct rand_dist *busy_dist,
					       unsigned int *rand_state,
					       struct cgroup *cgroup,
					       const struct cpumask *cpus)
{
	int i;
	int n_tasks = pick_n_tasks(rand_state);
	struct linsched_tg_sim *tgsim = malloc(sizeof(struct linsched_tg_sim));

	assert(tgsim);
	if (cgroup)
		tgsim->cg = cgroup;
	else
		tgsim->cg = linsched_create_cgroup(root_cgroup, NULL);

	sched_group_set_shares(cgroup_tg(tgsim->cg), shares);
	tgsim->n_tasks = n_tasks;
	tgsim->tasks = malloc(sizeof(struct task_struct*) * n_tasks);
	assert(tgsim->tasks);

	for (i = 0; i < n_tasks; i++) {
		struct task_data *td;

		if (!sleep_dist)
			sleep_dist = pick_random_sleep_dist(rand_state);
		else
			sleep_dist = linsched_copy_dist(sleep_dist, rand_state);
		if (!busy_dist)
			busy_dist = pick_random_run_dist(rand_state);
		else
			busy_dist = linsched_copy_dist(busy_dist, rand_state);

		td = linsched_create_rnd_dist_sleep_run(sleep_dist, busy_dist);
		tgsim->tasks[i] = linsched_create_normal_task(td, 0);
		set_cpus_allowed_ptr(tgsim->tasks[i], cpus);
		linsched_add_task_to_group(tgsim->tasks[i], tgsim->cg);
	}

	return tgsim;
}

void linsched_free_task_td(struct task_struct *p)
{
	struct task_data *td = task_thread_info(p)->td;
	if (!td)
		return;

	if (td->data) {
		struct rnd_dist_task *d = td->data;
		linsched_destroy_dist(d->busy_rdist);
		linsched_destroy_dist(d->sleep_rdist);
		free(d);
	}
	free(td);
}

void linsched_destroy_tg_sim(struct linsched_tg_sim *tgsim)
{
	int i;

	if (!tgsim)
		return;

	assert(tgsim->tasks);
	for (i=0;i < tgsim->n_tasks;i++)
		linsched_free_task_td(tgsim->tasks[i]);
	free(tgsim->tasks);
	free(tgsim);
}

static char *remove_prefix(char *s, char *prefix)
{
	size_t len = strlen(prefix);
	if (strncmp(s, prefix, len))
		return NULL;
	return s + len;
}

static enum RND_TYPE parse_type(char **line_ptr)
{
	int i;
	const static struct {
		char *name;
		enum RND_TYPE type;
	} types[] = { { "GAUSSIAN ", GAUSSIAN },
		      { "POISSON ", POISSON },
		      { "EXPONENTIAL ", EXPONENTIAL },
		      { "LOGNORMAL ", LOGNORMAL }};

	for(i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
		char *rest = remove_prefix(*line_ptr, types[i].name);
		if (rest) {
			*line_ptr = rest;
			return types[i].type;
		}
	}
	return MAX_RND_TYPE;
}

/* returns true if successful */
static int parse_double(char **line_ptr, double *val)
{
	int len;
	char *line = *line_ptr;
	if (sscanf(line, "%lf%n", val, &len) >= 1 &&
	    line[len] == ' ') {
		*line_ptr += len + 1;
		return 1;
	}
	return 0;
}

static struct rand_dist *parse_distribution(char **line_ptr, unsigned int *rand_state)
{
	enum RND_TYPE type = parse_type(line_ptr);
	double arg1, arg2;
	char *line = *line_ptr;

	/* make sure we don't initialize everything with the same seed */
	linsched_rand(rand_state);
	switch(type) {
	case GAUSSIAN:
		if (parse_double(&line, &arg1) && parse_double(&line, &arg2)) {
			*line_ptr = line;
			return linsched_init_gaussian(arg1, arg2, *rand_state);
		}
		return pick_gaussian_dist(rand_state);
	case POISSON:
		if (parse_double(&line, &arg1)) {
			*line_ptr = line;
			return linsched_init_poisson(arg1, *rand_state);
		}
		return pick_poisson_dist(rand_state);
	case EXPONENTIAL:
		if (parse_double(&line, &arg1)) {
			*line_ptr = line;
			return linsched_init_exponential(arg1, *rand_state);
		}
		return pick_exponential_dist(rand_state);
	case LOGNORMAL:
		if (parse_double(&line, &arg1) && parse_double(&line, &arg2)) {
			*line_ptr = line;
			return linsched_init_lognormal(arg1, arg2, *rand_state);
		}
		return pick_lognormal_dist(rand_state);
	default:
		return NULL;
	}
}

static struct cgroup *parse_fixed_cgroup(char **line_ptr)
{
	struct cgroup *res = NULL;
	char *rest;

	if ((rest = remove_prefix(*line_ptr, "ROOT "))) {
		res = root_cgroup;
	} else if ((rest = remove_prefix(*line_ptr, "ONE_GROUP "))) {
		res = linsched_create_cgroup(root_cgroup, "all_tasks");
	}
	if (rest)
		*line_ptr = rest;
	return res;
}

/* creates a linsched_sim object based on a shares file.
 * The structure of tg-shares file is specified as:
 * [ROOT |ONE_GROUP ]<N_TASK_GROUPS>
 * [sleep dist ][run dist ]<SHARES_OF_TG1>
 * [sleep dist ][run dist ]<SHARES_OF_TG2>
 * .......
 * .......
 * [sleep dist ][run dist ]<SHARES_OF_TGN>
 * Where a distribution is one of GUASSIAN, POISSON etc, with optional
 * parameters
 */
struct linsched_sim *linsched_create_sim(char *tg_file, const struct cpumask *cpus,
					 unsigned int *rand_state)
{
	FILE *tg_filp;
	char line[256];
	long shares;
	int n_tsk_grps;
	int i = 0;
	struct linsched_sim *lsim = malloc(sizeof(struct linsched_sim));
	struct linsched_tg_sim **tg_sim_arr = NULL;
	struct cgroup *group = NULL;

	if ((tg_filp = fopen(tg_file, "r")) != NULL) {
		if (fgets(line, sizeof(line), tg_filp)) {
			char *parsed_line = line;
			group = parse_fixed_cgroup(&parsed_line);
			n_tsk_grps = simple_strtoul(parsed_line, NULL, 0);
			if (n_tsk_grps)
				tg_sim_arr = malloc(n_tsk_grps *
					sizeof(struct linsched_tg_sim *));
		}
		if (!tg_sim_arr)
			return NULL;
		while (fgets(line, sizeof(line), tg_filp) && i < n_tsk_grps) {
			struct rand_dist *sleep_dist, *run_dist;
			char *parsed_line = line;

			sleep_dist = parse_distribution(&parsed_line, rand_state);
			run_dist = parse_distribution(&parsed_line, rand_state);
			shares = simple_strtoul(parsed_line, NULL, 0);
			tg_sim_arr[i++] =
				linsched_create_tg_sim(shares, sleep_dist,
						       run_dist, rand_state,
						       group, cpus);
		}
		lsim->n_task_grps = n_tsk_grps;
		lsim->tg_sim_arr = tg_sim_arr;
		fclose(tg_filp);
		return lsim;
	}
	return NULL;
}

void linsched_destroy_sim(struct linsched_sim *lsim)
{
	int i;

	if (!lsim)
		return;


	if (lsim->tg_sim_arr) {
		for (i = 0; i < lsim->n_task_grps; i++)
			if (lsim->tg_sim_arr[i])
				linsched_destroy_tg_sim(lsim->tg_sim_arr[i]);
		free(lsim->tg_sim_arr);
	}
	free(lsim);
}

void print_dist_params(struct rand_dist *rdist)
{
	enum RND_TYPE type = rdist->type;
	struct lognormal_dist *ldist;
	struct gaussian_dist *gdist;
	struct poisson_dist *pdist;
	struct exp_dist *edist;

	switch (type) {
	case LOGNORMAL:
		ldist = rdist->dist;

		fprintf(stdout, "LOGNORMAL: meanlog = %f, sdlog = %f",
				ldist->meanlog, ldist->sdlog);
		break;
	case GAUSSIAN:
		gdist = rdist->dist;
		fprintf(stdout, "GAUSSIAN: mean = %d, sd = %d",
					gdist->mu, gdist->sigma);
		break;
	case POISSON:
		pdist = rdist->dist;
		fprintf(stdout, "POISSON: mean = %d", pdist->mu);
		break;
	case EXPONENTIAL:
		edist = rdist->dist;
		fprintf(stdout, "EXPONENTIAL: mean = %d", edist->mu);
		break;
	}
}


void print_task_params(struct task_struct *p, struct cgroup *cgrp)
{
	struct task_data *td = task_thread_info(p)->td;
	struct rnd_dist_task *rd = td->data;
	int id = task_thread_info(rd->sr_data.p)->id;
	char buf[128];

	cgroup_path(cgrp, buf, 128);
	fprintf(stdout, "\nCGroup = %s, Task Id = %d ", buf, id);
	fprintf(stdout, "sleep_dist: ");
	print_dist_params(rd->sleep_rdist);
	fprintf(stdout, "\nCGroup = %s, Task Id = %d ", buf, id);
	fprintf(stdout, "busy_dist : ");
	print_dist_params(rd->busy_rdist);
}

void print_report(struct linsched_sim *lsim)
{
	int i, j;
	struct linsched_tg_sim *tgsim;

	linsched_print_task_stats();
	linsched_print_group_stats();

	/* for each task in each task_group, print its run / sleep params */

	for (i = 0; i < lsim->n_task_grps; i++) {
		tgsim = lsim->tg_sim_arr[i];

		for (j = 0; j < tgsim->n_tasks; j++)
			print_task_params(tgsim->tasks[j], tgsim->cg);
	}
	fprintf(stdout, "\n");
}
