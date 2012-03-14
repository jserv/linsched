/* Fractional CPU test for the Linux Scheduler Simulator
 *
 * A fractional CPU test that starts of two groups of tasks (a protagonist
 * and an antagonist) with varying CPU share allocations. The net effect
 * of the CPU alloction by the scheduler is measured by the amount of run
 * time alloted to each group at the end of the simulation.
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
#include <strings.h>
#include <getopt.h>
#include <stdio.h>


static int verbose_flag;

void print_usage(char *cmd)
{
	printf("Usage: %s --pthread p --athread a --pshares m --ashares c"
	       " --busy b --sleep s --duration\n", cmd);
	printf("\t\t --pthread[-p]: Number of protagonist threads\n");
	printf("\t\t --athread[-a]: Number of antagonist threads\n");
	printf("\t\t --pshares[-m]: Shares value for protagonist\n");
	printf("\t\t --ashares[-c]: Shares value for antagonist\n");
	printf("\t\t --busy[-b]: busy time for threads in ms\n");
	printf("\t\t --sleep[-s]: sleep time for threads in ms\n");
	printf("\t\t --duration[-l]: number of ticks (ms) to "
	       "run the simulation - default (60000)\n");
}

int linsched_test_main(int argc, char **argv)
{
	int i, c;
	struct task_struct *t;
	struct cgroup *protag, *antag;
	unsigned long pthreads = 0, athreads = 0;
	unsigned long pshares = 0, ashares = 0;
	unsigned long busy = 0, sleep = 0;
	unsigned long duration = 60000;

	while (1) {
		static struct option long_options[] = {
			{"verbose", no_argument, &verbose_flag, 1},
			{"brief", no_argument, &verbose_flag, 0},
			{"pthreads", required_argument, 0, 'p'},
			{"athreads", required_argument, 0, 'a'},
			{"pshares", required_argument, 0, 'm'},
			{"ashares", required_argument, 0, 'c'},
			{"busy", required_argument, 0, 'b'},
			{"sleep", required_argument, 0, 's'},
			{"duration", required_argument, 0, 'l'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long(argc, argv, "p:a:m:c:b:s:l",
				long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
		case 0:
			break;

		case 'p':
			pthreads = simple_strtol(optarg, NULL, 0);
			break;

		case 'a':
			athreads = simple_strtol(optarg, NULL, 0);
			break;

		case 'm':
			pshares = simple_strtol(optarg, NULL, 0);
			break;

		case 'c':
			ashares = simple_strtol(optarg, NULL, 0);
			break;

		case 'b':
			busy = simple_strtol(optarg, NULL, 0);
			break;

		case 's':
			sleep = simple_strtol(optarg, NULL, 0);
			break;

		case 'l':
			duration = simple_strtol(optarg, NULL, 0);
			break;

		case '?':
			/* getopt_long already printed an error message. */
			break;
		}
	}

	if (!pthreads || !athreads || !pshares || !ashares || !busy || !sleep) {
		print_usage(argv[0]);
		return 1;
	}

	/* Initialize linsched. */
	linsched_init(NULL);

	protag = linsched_create_cgroup(root_cgroup, "protag");
	sched_group_set_shares(cgroup_tg(protag), pshares);

	/* Create protagonist */
	for (i = 0; i < pthreads; i++) {
		t = linsched_create_normal_task(linsched_create_sleep_run
						 (sleep, busy), 0);
		linsched_add_task_to_group(t, protag);
	}

	/* Create antagonist */
	antag = linsched_create_cgroup(root_cgroup, "antag");
	sched_group_set_shares(cgroup_tg(antag), ashares);
	for (i = 0; i < athreads; i++) {
		t = linsched_create_normal_task(linsched_create_sleep_run
						 (sleep, busy), 0);
		linsched_add_task_to_group(t, antag);
	}

	/* Run simulation to completion. */
	linsched_run_sim(duration);

	/* we always want these */
	linsched_global_options.print_tasks = 1;
	linsched_global_options.print_cgroups = 1;

	return 0;
}
