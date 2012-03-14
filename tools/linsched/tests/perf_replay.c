/* Copyright 2011 Google Inc. All Rights Reserved.
 * Author: asr@google.com (Abhishek Srivastava)
 *
 * Simple demo program for replaying perf traces
 * processed into .rlog(s)
 */

#include "linsched.h"
#include <strings.h>
#include <stdio.h>

void usage(void)
{
	fprintf(stdout, "\nUsage: perf_replay \
			<PATH_TO_DIRECTORY_WITH_RLOGS> <SIM_DURATION>\n");
}

int linsched_test_main(int argc, char **argv)
{
	int perf_error = 0, duration;

	/* Initialize linsched. */
	linsched_init(NULL);

	if (argc == 3)
		perf_error = linsched_create_perf_tasks(argv[1]);
	else {
		fprintf(stderr, "\ninvalid number of arguments.Exiting ...\n");
		usage();
		return -1;
	}

	if (perf_error) {
		fprintf(stderr, "\nfailed to create perf tasks.Exiting ...\n");
		return -1;
	}

	/* Run simulation */
	duration = simple_strtol(argv[2], NULL, 0);
	linsched_run_sim(duration);

	linsched_show_schedstat();

	/* we always want these */
	linsched_global_options.print_tasks = 1;
	linsched_global_options.print_cgroups = 1;

	return 0;
}
