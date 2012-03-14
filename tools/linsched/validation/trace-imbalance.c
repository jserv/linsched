/*
 * Takes the output of perf script -D and runs load_balance_score.c
 * against it.
 *
 * Note that even using perf script event ordering occasionally breaks
 * or events get dropped, so the BUG_ONs are used to make sure such
 * runs get dropped.
 */

#define _BSD_SOURCE
#include "linsched.h"
#include "test_lib.h"
#include "load_balance_score.h"
#include "sanity_check.h"
#include <stdio.h>
#include <stdlib.h>
#include <pcre.h>

#ifdef NDEBUG
#define debugf(...) do { } while(0)
#else
#define debugf(...) printf(__VA_ARGS__)
#endif

struct pid_map {
	int pid;
	struct task_struct *p;
};

struct pid_map *pid_map;
int pid_map_size;

static void move_task(struct task_struct *p, int cpu)
{
	debugf("(%d) %d -> %d\n", task_thread_info(p)->id, task_cpu(p), cpu);
	if (p->on_rq) {
		linsched_force_migration(p, cpu, 0);
	}
}

static void sleep_task(struct task_struct *p)
{
	int cpu = task_cpu(p);
	struct rq *rq = cpu_rq(cpu);
	debugf("(%d) %d -> sleep\n", task_thread_info(p)->id, cpu);
	BUG_ON(!p->on_rq || p->state);
	linsched_change_cpu(cpu);
	if (p == rq->curr) {
		p->state = TASK_INTERRUPTIBLE;
		schedule();
	} else {
		/*
		 * Technically we should somehow force the task to run
		 * and then do the above. Just forcing a dequeue is enough
		 * to get load balance scoring though.
		 */

		p->state = TASK_INTERRUPTIBLE;
		p->sched_class->dequeue_task(rq, p, DEQUEUE_SLEEP);
		p->on_rq = 0;
		rq->nr_running--;
	}

}

static void wake_task(struct task_struct *p, int cpu)
{
	int ret;
	debugf("(%d) wake %d -> %d\n", task_thread_info(p)->id, task_cpu(p), cpu);

	if (p->on_rq && task_cpu(p) == cpu) {
		/*
		 * if we get an hrtimer interrupt between the hrtimer
		 * creation and the schedule() call, we get a spurious
		 * wakeup. Ignore it.
		 */
		return;
	}

	BUG_ON(!p->state || p->on_rq);
	linsched_change_cpu(cpu);
	set_cpus_allowed(p, cpumask_of_cpu(cpu));
	ret = wake_up_process(p);
	BUG_ON(!ret || p->state || !p->on_rq);
}

struct cgroup *find_cgroup(char *target)
{
	int i, shares;
	char path[512];
	struct cgroup *cg;
	FILE *f;
	extern int num_cgroups; /* defined in linux_linsched.c */

	for (i = 0; i < num_cgroups; i++) {
		cg = &__linsched_cgroups[i].cg;
		if (cgroup_path(cg, path, sizeof(path)))
			continue;
		if (!strcmp(path, target))
			return cg;
	}
	/* only support one level deep for now */
	BUG_ON(strchr(target + 1, '/'));
	snprintf(path, sizeof(path), "/dev/cgroup/cpu%s/cpu.shares", target);
	f = fopen(path, "r");
	if (!f || fscanf(f, "%d", &shares) != 1)
		return NULL;
	fclose(f);
	cg = linsched_create_cgroup(root_cgroup, target + 1);
	sched_group_set_shares(cgroup_tg(cg), shares);
	return cg;
}

int find_cpu_cgroup(void)
{
	FILE *f;
	char line[256];
	int id;
	f = fopen("/proc/cgroups", "r");
	BUG_ON(!f);

	while (fgets(line, sizeof(line), f)) {
		if (sscanf(line, "cpu %d %*d 1", &id) == 1) {
			return id;
		}
	}
	BUG();
	return -1;
}

struct pid_map *pid_find(int pid)
{
	int i = pid % pid_map_size;
	while (pid_map[i].pid != pid && pid_map[i].pid) {
		i++;
		i %= pid_map_size;
	}
	return &pid_map[i];
}

pcre *re_comp(const char *regex) {
	const char *err_str;
	int err_place;
	pcre *res = pcre_compile(regex, 0, &err_str, &err_place, NULL);
	if (!res) {
		printf("%s: at %d in %s\n", err_str, err_place, regex);
		BUG();
	}
	return res;
}

int re_exec(pcre *re, char *str, int *ovec, int ovecsize) {
	int res = pcre_exec(re, NULL, str, strlen(str), 0, 0, ovec, ovecsize);
	BUG_ON(res < 0 && res != PCRE_ERROR_NOMATCH);
	return res;
}

char *group(char *line, int *ovec, int group)
{
	int start = ovec[group * 2];
	int end = ovec[group * 2 + 1];
	BUG_ON(start < 0 || end < 0);
	line[end] = '\0';
	return &line[start];
}

/* Now that everything is set up parse stdin from perf script -D
 * format into calls to wake_task/sleep_task/move_task */
void run_input(void)
{
	pcre *timestamp_re, *common_re, *switch_re, *migrate_re, *wakeup_re;
	u64 next_ns = 0;
	char line[512];

	timestamp_re = re_comp("[0-9]+ ([0-9]+) 0x.*: PERF_RECORD_SAMPLE.*");

	common_re = re_comp("^.* \\[([0-9]*)\\]  *([0-9]*)\\.([0-9]*):"
		" ([^:]*): (.*)$");

	wakeup_re = re_comp("^comm=.* pid=([0-9]*) .* target_cpu=*([0-9]*)");
	switch_re = re_comp("^prev_comm.* prev_pid=([0-9]*) .* prev_state=[^R ]+"
		" ==> next_comm.*");
	migrate_re = re_comp("^comm.* pid=([0-9]*) .* dest_cpu=([0-9]*)");

	while(fgets(line, sizeof(line), stdin)) {
		struct pid_map *pid;
		char *tail, *name;
		int ovec[30];
		int res;
		long long secs, usecs;
		int cpu;
		tail = strchr(line, '\n');
		if (tail)
			*tail = '\0';

		res = re_exec(timestamp_re, line, ovec, ARRAY_SIZE(ovec));
		if (res > 0) {
			next_ns = strtoull(group(line, ovec, 1), NULL, 10);
			continue;
		}

		res = re_exec(common_re, line, ovec, ARRAY_SIZE(ovec));
		if (res < 0)
			continue;

		cpu = atoi(group(line, ovec, 1));
		secs = atoll(group(line, ovec, 2));
		usecs = atoll(group(line, ovec, 3));
		name = group(line, ovec, 4);
		tail = group(line, ovec, 5);
		current_time = secs * 1000000000ULL + usecs * 1000;
		if (next_ns > current_time && next_ns - current_time < 1000)
			current_time = next_ns;
		next_ns = 0;

		if (!strcmp(name, "sched_wakeup")) {
			res = re_exec(wakeup_re, tail, ovec, ARRAY_SIZE(ovec));
			BUG_ON(res < 0);
			pid = pid_find(atol(group(tail, ovec, 1)));
			cpu = atoi(group(tail, ovec, 2));
			if (!pid->p)
				continue;

			wake_task(pid->p, cpu);
		} else if(!strcmp(name, "sched_switch")) {
			res = re_exec(switch_re, tail, ovec, ARRAY_SIZE(ovec));
			if (res < 0)
				continue;
			pid = pid_find(atol(group(tail, ovec, 1)));
			if (!pid->p)
				continue;

			sleep_task(pid->p);
		} else if (!strcmp(name, "sched_migrate_task")) {
			res = re_exec(migrate_re, tail, ovec, ARRAY_SIZE(ovec));
			BUG_ON(res < 0);
			pid = pid_find(atol(group(tail, ovec, 1)));
			cpu = atoi(group(tail, ovec, 2));
			if (!pid->p)
				continue;

			move_task(pid->p, cpu);
		}

		linsched_enable_migrations();
		compute_lb_info();
		for_each_online_cpu(cpu) {
			linsched_change_cpu(cpu);
			run_sanity_check();
		}
		linsched_disable_migrations();
	}
}

int linsched_test_main(int argc, char **argv)
{
	struct linsched_topology topo;
	struct cpumask monitor_cpus;
	int cpu, i;
	int cpu_cgroup;
	cpumask_var_t *doms = alloc_sched_domains(2);

	if (argc < 4) {
		fprintf(stderr, "Usage: %s <topology> <monitoring cpus> <pid> ...\n", argv[0]);
		return 1;
	}

	topo = linsched_topo_db[parse_topology(argv[1])];

	linsched_init(&topo);

	if (cpulist_parse(argv[2], &monitor_cpus)) {
		BUG();
	}

	cpu_cgroup = find_cpu_cgroup();

	/* Turn off the cpus used for monitoring. This is entirely
	 * equivalent for our purposes to the cpuset-like that
	 * mcarlo-sim does. */
	for_each_cpu(cpu, &monitor_cpus) {
		linsched_offline_cpu(cpu);
	}

	pid_map_size = (argc - 3) * 2; /* 2 * npids */
	pid_map = calloc(pid_map_size, sizeof(struct pid_map));

	for (i = 3; i < argc; i++) {
		char line[512];
		FILE *f;
		int pid = atoi(argv[i]);
		struct pid_map *pmap;
		struct task_data *td = linsched_create_sleep_run(0, 10);
		struct task_struct *p = linsched_create_normal_task(td, 0);
		struct cgroup *cg = NULL;
		snprintf(line, sizeof(line), "/proc/%d/cgroup", pid);
		f = fopen(line, "r");
		BUG_ON(!f);
		while (fgets(line, sizeof(line), f)) {
			int id;
			int len;
			if (sscanf(line, "%d:%*[^:]:%n", &id, &len) < 1)
				continue;
			if (id == cpu_cgroup) {
				char *nl = strchr(line, '\n');
				if (nl)
					*nl = '\0';
				cg = find_cgroup(&line[len]);
				break;
			}
		}
		BUG_ON(!cg);
		linsched_add_task_to_group(p, cg);
		sleep_task(p);

		pmap = pid_find(pid);
		pmap->p = p;
		pmap->pid = pid;
	}

	linsched_disable_migrations();

	run_input();

	printf("average imbalance: %f\n", get_average_imbalance());
	return 0;
}
