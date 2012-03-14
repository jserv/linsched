#include "linsched.h"
#include "linsched_rand.h"
#include <strings.h>

#define LINSCHED_TICKS 60000

int linsched_test_main(int argc, char **argv)
{
	struct task_struct *t1, *t2;
	struct cgroup *g1;

	struct rand_dist *pdist, *gdist;
	int seed = getticks();
	int sleep_mu = 400, sleep_sigma = 20;
	int busy_mu = 100;

	/* Initialize linsched. */
	linsched_init(NULL);

	/* create a gaussian sleep dist */
	gdist = linsched_init_gaussian(sleep_mu, sleep_sigma, seed);

	/* create a poisson dist */
	pdist = linsched_init_poisson(busy_mu, seed);

	t1 = linsched_create_normal_task(
			linsched_create_rnd_dist_sleep_run(gdist, pdist), 0);

	g1 = linsched_create_cgroup(root_cgroup, "g1");
	sched_group_set_shares(cgroup_tg(g1), 2048);
	linsched_add_task_to_group(t1, g1);

	linsched_create_RTrr_task(linsched_create_rnd_dist_sleep_run
				  (pdist, gdist), 0);

	/* create more tasks here... */

	t2 = linsched_create_RTfifo_task(linsched_create_sleep_run
					 (8, 2), 0);
	linsched_add_task_to_group(t2, g1);

	linsched_create_normal_task(linsched_create_rnd_dist_sleep_run
				    (gdist, gdist), 0);

	/* Run simulation for 500 ticks. */
	linsched_run_sim(500);

	/* Run simulation to completion. */
	linsched_run_sim(LINSCHED_TICKS - 500);

	/* we always want these */
	linsched_global_options.print_tasks = 1;
	linsched_global_options.print_cgroups = 1;

	/* cleanup */
	linsched_destroy_gaussian(gdist);
	linsched_destroy_poisson(pdist);

	return 0;
}
