/*
 * Runs the equivalent of mcarlo-sim on the current machine instead of
 * in simulation. Requires root.
 *
 * Prints the equivalent of --print_cgroup_stats
 * --print_average_imbalance --print_sched_stats
 *
 * Requires the arguments -c to specify a cpu list to run the tests
 * on, and -m to specify the cpus to monitor from, as well as the
 * normal mcarlo-sim ones.
 *
 * Depends on the scripts schedstat-diff.py, unix-mcarlo-sim-init.sh,
 * and unix-mcarlo-sim-monitor.sh being in the same directory.
 */

#define __STRICT_ANSI__
#include <errno.h>
#undef __always_inline
#include "linsched.h"
#include "linsched_rand.h"
#include "linsched_sim.h"
#include "test_lib.h"
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#define _SYS_RESOURCE_H
#include <sys/wait.h>

/* Faked header include due to conflicts with linux_sched_headers.h */
/* #include <time.h> */
int nanosleep(const struct timespec *req, struct timespec *rem);
int clock_gettime(clockid_t clk_id, struct timespec *tp);
/* #include <signal.h> */
int killpg(int pgrp, int sig);
int kill(pid_t pid, int sig);
/* #include <sys/stat.h> */
int mkdir(const char *pathname, mode_t mode);


static char exec_dir[512];

s64 time_sub(struct timespec *end, struct timespec *start)
{
	return (end->tv_sec - start->tv_sec) * NSEC_PER_SEC +
		end->tv_nsec - start->tv_nsec;
}

void sleep_cycle(u64 ns)
{
	struct timespec dur = {0, 0};
	struct timespec rem = { ns / NSEC_PER_SEC, ns % NSEC_PER_SEC };
	int rc = 0;

	do {
		dur = rem;
		rc = nanosleep(&dur, &rem);
	} while(rc == -1 && errno == EINTR);
}

void busy_cycle(u64 ns)
{
	struct timespec start, prev, cur;
	clock_gettime(CLOCK_MONOTONIC, &start);
	cur = start;
	do {
		long delta;
		prev = cur;
		clock_gettime(CLOCK_MONOTONIC, &cur);
		delta = time_sub(&cur, &prev);
		/* assume skips of 2+ microseconds are due to context switch */
		if(delta > 2000) {
			ns += delta;
		}
	} while(time_sub(&cur, &start) < ns);
}

void run_rand_dist(struct rnd_dist_task *d)
{
	struct rand_dist *sdist = d->sleep_rdist;
	struct rand_dist *bdist = d->busy_rdist;

	kill(getpid(), SIGSTOP);

	while(1) {
		busy_cycle(d->busy);
		d->sleep = sdist->gen_fn(sdist);
		d->busy = bdist->gen_fn(bdist);
		if (!d->sleep)
			d->sleep = 1;
		sleep_cycle(d->sleep);
	}
}



void print_usage(char *cmd)
{
	printf("Usage: %s -c <cpulist> -m <cpulist> -f <SHARES_FILE>"
	       " --duration <SIMDUARATION> -t <topology> [-s seed]\n", cmd);
}

void append_file(char *fn, char *contents)
{
	FILE *f = fopen(fn, "a");
	fputs(contents, f);
	fclose(f);
}

void write_file(char *fn, char *contents)
{
	FILE *f = fopen(fn, "a");
	fputs(contents, f);
	fclose(f);
}


void run_mcarlo_sim(struct linsched_sim *sim, char *cpus, char *monitor_cpus,
		    char *duration, char *topo)
{
	int group;
	char buf[512];
	int pid = getpid();
	int ret;
	int total_tasks = 0;

	/* invoke half of this via a shell script to avoid painful
	 * directory traversal in C */

	sprintf(buf, "%s/unix-mcarlo-sim-init.sh %s %s", exec_dir,
		cpus, monitor_cpus);
	ret = system(buf);
	if (ret) {
		if (ret == -1) {
			int err = errno;
			fprintf(stderr, "Unable to run '%s': ", buf);
			errno = err;
			perror(NULL);
		}
		exit(1);
	}

	sprintf(buf, "%d", pid);
	append_file("/dev/cgroup/cpuset/test/tasks", buf);
	for(group = 0; group < sim->n_task_grps; group++) {
		char cg_base[512], path[512];
		struct linsched_tg_sim *tgsim = sim->tg_sim_arr[group];
		int task;
		cgroup_path(tgsim->cg, buf, sizeof(buf));
		sprintf(cg_base, "/dev/cgroup/cpu%s", buf);

		mkdir(cg_base, 0777);

		sprintf(buf, "%lu", cgroup_tg(tgsim->cg)->shares);
		sprintf(path, "%s/cpu.shares", cg_base);
		write_file(path, buf);

		sprintf(buf, "%d", pid);
		sprintf(path, "%s/tasks", cg_base);
		append_file(path, buf);

		for(task = 0; task < tgsim->n_tasks; task++) {
			if(!fork()) {
				struct task_data *td;
				/* child automatically in the correct cgroups */

				td = task_thread_info(tgsim->tasks[task])->td;
				run_rand_dist(td->data);
			}
			total_tasks++;
		}
	}
	sprintf(buf, "%d", pid);
	append_file("/dev/cgroup/cpuset/monitor/tasks", buf);
	append_file("/dev/cgroup/cpu/tasks", buf);

	while(total_tasks) {
		int status;
		pid_t cpid;
		cpid = waitpid(0, &status, WUNTRACED);
		if (cpid == -1) {
			perror("failed waitpid");
			exit(1);
		}
		if (!WIFSTOPPED(status)) {
			char *msg = "??? with raw status";
			int num = status;

			if (WIFEXITED(status)) {
				msg = "exited with code";
				num = WEXITSTATUS(status);
			} else if (WIFSIGNALED(status)) {
				msg = "killed by signal";
				num = WTERMSIG(status);
			}
			fprintf(stderr, "Child %d %s %d", cpid, msg, num);
			killpg(0, SIGTERM);
			exit(1);
		}
		total_tasks--;
	}

	sprintf(buf, "%s/unix-mcarlo-sim-monitor.sh", exec_dir);
	ret = execl(buf, buf, duration, topo, monitor_cpus, NULL);
	perror("execl of monitoring script failed");
	exit(1);
}

int linsched_test_main(int argc, char **argv)
{
	int c;
	char tg_file[512] = "", cpus[64] = "", monitor_cpus[64] = "";
	char duration[64] = "", topo[64] = "";
	unsigned int seed = getticks();
	char *slash;

	strcpy(exec_dir, argv[0]);
	slash = strrchr(exec_dir, '/');
	if (slash) {
		*slash = '\0';
	} else {
		strcpy(exec_dir, ".");
	}

	while (1) {
		static struct option const long_options[] = {
			{"cpus", required_argument, 0, 'c'},
			{"monitor_cpus", required_argument, 0, 'm'},
			{"topo", required_argument, 0, 't'},
			{"tg_file", required_argument, 0, 'f'},
			{"duration", required_argument, 0, 'd'},
			{"seed", required_argument, 0, 's'},
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
		case 'c':
			strcpy(cpus, optarg);
			break;

		case 'm':
			strcpy(monitor_cpus, optarg);
			break;

		case 'f':
			strcpy(tg_file, optarg);
			break;

		case 'd':
			strcpy(duration, optarg);
			break;
		case 't':
			strcpy(topo, optarg);
			break;
		case 's':
			seed = simple_strtoul(optarg, NULL, 0);
			break;
		case '?':
			/* getopt_long already printed an error message. */
			break;
		}
	}

	if (strcmp(cpus, "") && strcmp(monitor_cpus, "") &&
	    strcmp(tg_file, "") && strcmp(duration, "") &&
	    strcmp(topo, "")) {
		fprintf(stdout, "\nMonitoring cpus = %s, test cpus = %s, "
			"tg_file = %s, duration = %s\n",
			monitor_cpus, cpus, tg_file, duration);
		struct linsched_topology ltopo = TOPO_UNIPROCESSOR;
		struct linsched_sim *lsim;
		unsigned int *rand_state = linsched_init_rand(seed);

		linsched_init(&ltopo);
		lsim = linsched_create_sim(tg_file, cpu_online_mask, rand_state);
		if (lsim) {
			run_mcarlo_sim(lsim, cpus, monitor_cpus, duration, topo);
		} else {
			fprintf(stderr, "failed to create simulation.\n");
			print_usage("unix-mcarlo-sim");
		}
	} else {
		print_usage(argv[0]);
	}

	return 0;
}
