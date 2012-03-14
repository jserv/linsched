/* Copyright 2011 Google Inc. All Rights Reserved.
 * Author: asr@google.com (Abhishek Srivastava)
 *
 * Fractional CPU test using tasks following normal
 * and poisson sleep / run time distributions.
 *
 */

#include "linsched.h"
#include "linsched_rand.h"
#include <strings.h>
#include <getopt.h>
#include <stdio.h>
#include <error.h>

static int verbose_flag;

void print_usage(char *cmd)
{
	printf("Usage: %s --pthread p --athread a --pshares m --ashares c"
	       " --busy b --sleep s --duration\n", cmd);
	printf("\t\t --pthread[-p]: Number of protagonist threads\n");
	printf("\t\t --athread[-a]: Number of antagonist threads\n");
	printf("\t\t --pshares[-m]: Shares value for protagonist\n");
	printf("\t\t --ashares[-c]: Shares value for antagonist\n");
	printf("\t\t --p_busy_mu[-b]: busy mu for protagonists\n");
	printf("\t\t --p_sleep_mu[-s]: sleep mu for protagonists\n");
	printf("\t\t --p_busy_sigma[-g]: busy sigma for protagonists\n");
	printf("\t\t --p_sleep_sigma[-h]: sleep sigma for protagonists\n");
	printf("\t\t --a_busy_mu[-j]: busy mu for antagonists\n");
	printf("\t\t --a_sleep_mu[-k]: sleep mu for antagonists\n");
	printf("\t\t --a_busy_sigma[-z]: busy sigma for antagonists\n");
	printf("\t\t --a_sleep_sigma[-d]: sleep sigma for antagonists\n");
	printf("\t\t --prot_rnd_type[-t]: protagonist rnd dist type\n");
	printf("\t\t --anta_rnd_type[-y]: antagonist rnd dist type\n");
	printf("\t\t --duration[-l]: number of ticks (ms) to "
	       "run the simulation - default (60000)\n");
	printf("\t\t rnd distribution type : \n");
	printf("\t\t\t 1. busy = gaussian, sleep = gaussian\n");
	printf("\t\t\t 2. busy = poisson, sleep = gaussian\n");
	printf("\t\t\t 3. busy = gaussian, sleep = poisson\n");
	printf("\t\t\t 4. busy = poisson, sleep = poisson\n");
	printf("\t\t all sleep / busy times to be specified in ms\n");

}

int linsched_test_main(int argc, char **argv)
{
	int i, c;
	struct task_struct *t;
	struct cgroup *protag, *antag;
	unsigned long pthreads = 0, athreads = 0;
	unsigned long pshares = 0, ashares = 0;
	unsigned long duration = 60000;
	unsigned int p_sleep_mu = 0, p_busy_mu = 0, p_sleep_sigma =
	    0, p_busy_sigma = 0;
	unsigned int a_sleep_mu = 0, a_busy_mu = 0, a_sleep_sigma =
	    0, a_busy_sigma = 0;
	unsigned int seed = getticks();
	struct rand_dist *busy_distA, *sleep_distA;
	struct rand_dist *busy_distP, *sleep_distP;
	enum RND_TYPE sleepA_type = -1, busyA_type = -1, sleepP_type = -1, busyP_type = -1;

	while (1) {
		static struct option long_options[] = {
			{"verbose", no_argument, &verbose_flag, 1},
			{"brief", no_argument, &verbose_flag, 0},
			{"pthreads", required_argument, 0, 'p'},
			{"athreads", required_argument, 0, 'a'},
			{"pshares", required_argument, 0, 'm'},
			{"ashares", required_argument, 0, 'c'},
			{"p_busy_mu", required_argument, 0, 'b'},
			{"p_sleep_mu", required_argument, 0, 's'},
			{"p_busy_sigma", required_argument, 0, 'g'},
			{"p_sleep_sigma", required_argument, 0, 'h'},
			{"a_busy_mu", required_argument, 0, 'j'},
			{"a_sleep_mu", required_argument, 0, 'k'},
			{"a_busy_sigma", required_argument, 0, 'z'},
			{"a_sleep_sigma", required_argument, 0, 'd'},
			{"prot_rnd_type", required_argument, 0, 't'},
			{"anta_rnd_type", required_argument, 0, 'y'},
			{"duration", required_argument, 0, 'l'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long(argc, argv,
				"p:a:m:c:b:s:g:h:j:k:z:d:t:y:l",
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
			p_busy_mu = simple_strtol(optarg, NULL, 0);
			break;

		case 's':
			p_sleep_mu = simple_strtol(optarg, NULL, 0);
			break;

		case 'g':
			p_busy_sigma = simple_strtol(optarg, NULL, 0);
			break;

		case 'h':
			p_sleep_sigma = simple_strtol(optarg, NULL, 0);
			break;

		case 'j':
			a_busy_mu = simple_strtol(optarg, NULL, 0);
			break;

		case 'k':
			a_sleep_mu = simple_strtol(optarg, NULL, 0);
			break;

		case 'z':
			a_busy_sigma = simple_strtol(optarg, NULL, 0);
			break;

		case 'd':
			a_sleep_sigma = simple_strtol(optarg, NULL, 0);
			break;

		case 't':
			i = simple_strtol(optarg, NULL, 0);
			switch (i) {
			case 1:
				sleepP_type = GAUSSIAN;
				busyP_type = GAUSSIAN;
				break;
			case 2:
				sleepP_type = GAUSSIAN;
				busyP_type = POISSON;
				break;
			case 3:
				sleepP_type = POISSON;
				busyP_type = GAUSSIAN;
				break;
			case 4:
				sleepP_type = POISSON;
				busyP_type = POISSON;
				break;
			default:
				/* default to normal dist for both */
				sleepP_type = GAUSSIAN;
				busyP_type = GAUSSIAN;
				break;
			}
			break;

		case 'y':
			i = simple_strtol(optarg, NULL, 0);
			switch (i) {
			case 1:
				sleepA_type = GAUSSIAN;
				busyA_type = GAUSSIAN;
				break;
			case 2:
				sleepA_type = GAUSSIAN;
				busyA_type = POISSON;
				break;
			case 3:
				sleepA_type = POISSON;
				busyA_type = GAUSSIAN;
				break;
			case 4:
				sleepA_type = POISSON;
				busyA_type = POISSON;
				break;
			default:
				/* default to normal dist for both */
				sleepA_type = GAUSSIAN;
				busyA_type = GAUSSIAN;
				break;
			}
			break;

		case 'l':
			duration = simple_strtol(optarg, NULL, 0);
			break;

		case '?':
			/* getopt_long already printed an error message. */
			break;
		}
	}

	if (!pthreads || !athreads || !pshares || !ashares || !p_busy_mu
	    || !p_sleep_mu || !p_busy_sigma || !p_sleep_sigma || !a_busy_mu
	    || !a_sleep_mu || !a_sleep_sigma || !a_busy_sigma) {
		print_usage(argv[0]);
		return 1;
	}

	/* Initialize linsched. */
	linsched_init(NULL);

	protag = linsched_create_cgroup(root_cgroup, "protag");
	sched_group_set_shares(cgroup_tg(protag), pshares);

	/* create the distributions */
	seed = getticks();

	switch (busyP_type) {
	case GAUSSIAN:
		busy_distP =
		    linsched_init_gaussian(p_busy_mu, p_busy_sigma, seed);
		break;
	case POISSON:
		busy_distP = linsched_init_poisson(p_busy_mu, seed);
		break;
	default:
		error(1, 0, "unrecognized distribution: %d\n", busyP_type);
	}
	switch (sleepP_type) {
	case GAUSSIAN:
		sleep_distP =
		    linsched_init_gaussian(p_sleep_mu, p_sleep_sigma,
					   seed);
		break;
	case POISSON:
		sleep_distP = linsched_init_poisson(p_sleep_mu, seed);
		break;
	default:
		error(1, 0, "unrecognized distribution: %d\n", sleepP_type);
	}

	switch (busyA_type) {
	case GAUSSIAN:
		busy_distA =
		    linsched_init_gaussian(a_busy_mu, a_busy_sigma, seed);
		break;
	case POISSON:
		busy_distA = linsched_init_poisson(a_busy_mu, seed);
		break;
	default:
		error(1, 0, "unrecognized distribution: %d\n", busyA_type);
	}
	switch (sleepA_type) {
	case GAUSSIAN:
		sleep_distA =
		    linsched_init_gaussian(a_sleep_mu, a_sleep_sigma,
					   seed);
		break;
	case POISSON:
		sleep_distA = linsched_init_poisson(a_sleep_mu, seed);
		break;
	default:
		error(1, 0, "unrecognized distribution: %d\n", sleepA_type);
	}

	/* Create protagonist */
	for (i = 0; i < pthreads; i++) {
		t = linsched_create_normal_task
		    (linsched_create_rnd_dist_sleep_run
		     (sleep_distP, busy_distP), 0);
		linsched_add_task_to_group(t, protag);
	}

	/* Create antagonist */
	antag = linsched_create_cgroup(root_cgroup, "antag");
	sched_group_set_shares(cgroup_tg(protag), ashares);
	for (i = 0; i < athreads; i++) {
		t = linsched_create_normal_task
		    (linsched_create_rnd_dist_sleep_run
		     (sleep_distA, busy_distA), 0);
		linsched_add_task_to_group(t, antag);
	}

	/* Run simulation to completion. */
	linsched_run_sim(duration);

	/* we always want these */
	linsched_global_options.print_tasks = 1;
	linsched_global_options.print_cgroups = 1;

	return 0;
}
