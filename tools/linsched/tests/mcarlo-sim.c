/* Monte Carlo simulation of task models for Linsched
 *
 * The simulation simulates various combinations of task models within task
 * groups. Given an architecture, the number of task groups to simulate
 * and the shares for each task group, the simulation creates random number of
 * tasks with random run / sleep distributions chosen from the set of available
 * distributions - lognormal, normal, poisson, exponential. After the simulation,
 * a report is generated that prints out all the task and group statistics for
 * each run of the simulation. Presently this simulation does not support
 * hierarchies of task groups -- just flat task groups.
 */

#include "linsched.h"
#include "linsched_rand.h"
#include "linsched_sim.h"
#include "test_lib.h"
#include <string.h>
#include <getopt.h>
#include <stdio.h>

void print_usage(char *cmd)
{
	printf("Usage: %s -t <topo> -f <SHARES_FILE>"
	       " --duration <SIMDUARATION> [-c <cpus> -m <monitor_cpus>] [-s seed]\n", cmd);
}

void run_mcarlo_sim(char *stopo, char *tg_file, int simduration,
		    unsigned int seed, struct cpumask *cpus,
		    struct cpumask *monitor_cpus)
{
	struct linsched_topology topo = linsched_topo_db[parse_topology(stopo)];
	struct linsched_sim *lsim;
	unsigned int *rand_state = linsched_init_rand(seed);

	linsched_init(&topo);
	lsim = linsched_create_sim(tg_file, cpus, rand_state);

	if (!cpumask_subset(cpu_online_mask, cpus)) {
		/* split the system into two "cpuset"s with default attrs */
		cpumask_var_t *doms = alloc_sched_domains(2);
		cpumask_copy(doms[0], cpus);
		cpumask_copy(doms[1], monitor_cpus);
		partition_sched_domains(2, doms, NULL);
	}

	if (lsim) {
		linsched_run_sim(simduration);
		print_report(lsim);
		linsched_destroy_sim(lsim);
	} else {
		fprintf(stderr, "failed to create simulation.\n");
		print_usage("mcarlo-sim");
	}
}

int linsched_test_main(int argc, char **argv)
{
	int c, simduration = 0;
	char tg_file[256] = "", topo[256] = "";
	unsigned int seed = getticks();

	struct cpumask cpus = CPU_MASK_ALL, monitor_cpus = CPU_MASK_NONE;

	while (1) {
		static struct option const long_options[] = {
			{"topo", required_argument, 0, 't'},
			{"tg_file", required_argument, 0, 'f'},
			{"duration", required_argument, 0, 'd'},
			{"cpus", required_argument, 0, 'c'},
			{"monitor_cpus", required_argument, 0, 'm'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long(argc, argv, "t:f:d:s:c:m:",
					 long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {

		case 't':
			strcpy(topo, optarg);
			break;

		case 'f':
			strcpy(tg_file, optarg);
			break;

		case 'd':
			simduration = simple_strtoul(optarg, NULL, 0);
			break;
		case 's':
			seed = simple_strtoul(optarg, NULL, 0);
			break;
		case 'c':
			cpulist_parse(optarg, &cpus);
			break;
		case 'm':
			cpulist_parse(optarg, &monitor_cpus);
			break;
		case '?':
			/* getopt_long already printed an error message. */
			break;
		}
	}

	if (strcmp(topo, "") && strcmp(tg_file, "") && simduration &&
	    !cpumask_intersects(&cpus, &monitor_cpus)) {
		fprintf(stdout, "\nTOPO = %s, tg_file = %s, duration = %d\n",
				topo, tg_file, simduration);
		run_mcarlo_sim(topo, tg_file, simduration, seed, &cpus, &monitor_cpus);
	} else
		print_usage(argv[0]);

	return 0;
}
