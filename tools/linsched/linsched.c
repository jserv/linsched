#include "linsched.h"
#include "nohz_tracking.h"
#include "load_balance_score.h"

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

struct linsched_global_options linsched_global_options = {0};

static void print_global_usage(void)
{
	printf("Global options:\n");
	printf("\t\t --help_global_options: show this help\n");
	printf("\t\t --print_task_stats: print task runtime stats\n");
	printf("\t\t --print_cgroup_stats: print cgroup runtime stats\n");
	printf("\t\t --print_nohz_stats: print nohz residency information\n");
	printf("\t\t --print_average_imbalance: print average balance stats\n");
	printf("\t\t --dump_imbalance: print imbalance every step\n");
	printf("\t\t --dump_full_balance: print full load balance info"
	       "every step\n");
	printf("\n");
	exit(1);
}

void linsched_process_global_options(int *argc, char **argv)
{
	int idx = -2, c, i;
	struct linsched_global_options *opt = &linsched_global_options;

	struct option long_options[] = {
		{"help_global_options", no_argument, NULL, 'V' },
		{"print_nohz_stats", no_argument, &opt->print_nohz, 1},
		{"print_task_stats", no_argument, &opt->print_tasks, 1},
		{"print_cgroup_stats", no_argument, &opt->print_cgroups, 1},
		{"print_average_imbalance", no_argument, &opt->print_avg_imb, 1},
		{"print_sched_stats", no_argument, &opt->print_sched_stats, 1},
		{"dump_imbalance", no_argument, &opt->dump_imbalance, 1},
		{"dump_full_balance", no_argument, &opt->dump_full_balance, 1},
		{0, 0, 0, 0}
	};

	opterr = 0;
	while (1) {
		c = getopt_long(*argc, argv, "-", long_options, &idx);
		if (c == 'V') {
			print_global_usage();
		} else if (c == 0) {
			/*
			 * pull opt out of args so that it doesn't confuse
			 * other handlers or cause false negatives (e.g.
			 * invalid argument).
			 */
			for (i = --optind; i < *argc; i++)
				argv[i] = argv[i+1];
			(*argc)--;
		} else if (c == -1)
			break;
	}

	/* reset optind/opterr so that any other optarg handlers just 'work' */
	optind = 1;
	opterr = 1;
}

static void stat_header(const char *stat_name) {
	printf("------ %s\n", stat_name);
}

int linsched_test_main(int argc, char **argv);

int main(int argc, char **argv)
{
	int ret;

	linsched_process_global_options(&argc, argv);

	ret = linsched_test_main(argc, argv);

	if (linsched_global_options.print_tasks) {
		stat_header("task runtime");
		linsched_print_task_stats();
	}
	if (linsched_global_options.print_cgroups) {
		stat_header("group runtime");
		linsched_print_group_stats();
	}
	if (linsched_global_options.print_sched_stats) {
		stat_header("sched stats");
		linsched_show_schedstat();
	}
	if (linsched_global_options.print_nohz) {
		stat_header("nohz residency");
		print_nohz_residency();
	}
	if (linsched_global_options.print_avg_imb) {
		printf("average imbalance: %f\n", get_average_imbalance());
	}
	return ret;
}
