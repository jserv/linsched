/* Load balance scoring */

#include "test_lib.h"
#include "load_balance_score.h"
#include "lib/sort.h"
#include <stdio.h>
#include <malloc.h>
#include <math.h>

/* stdlib.h conflicts with linux/sched.h unfortunately */
void qsort(void *base, size_t nmemb, size_t size,
	   int(*compar)(const void *, const void *));

struct lb_group {
	struct task_group *tg;
	double h_weight;
	double total_load;
};
struct lb_task {
	struct task_struct *p;
	int cpus_allowed;
	double h_weight;
};

struct lb_cpu {
	int id;
	double load;
};

typedef uintptr_t hash_t;

struct hash_bucket {
	hash_t key;
	double value;
};

#define HASH_TABLE_SIZE 1023
static struct hash_bucket hash_table[HASH_TABLE_SIZE];

static struct lb_task lb_tasks[LINSCHED_MAX_TASKS];
static struct lb_group lb_groups[LINSCHED_MAX_GROUPS];
static int n_lb_tasks;
static int n_lb_groups;

static u64 last_time;
static u64 start_time;

static double total_imbalance;

static int cpu_load_compare(const struct lb_cpu *a, const struct lb_cpu *b);
static int task_compare(const struct lb_task *a, const struct lb_task *b);
define_specialized_qsort(tasks, struct lb_task, task_compare);
define_specialized_qsort(cpus, struct lb_cpu, cpu_load_compare);

static void split_h_weight(struct lb_group *parent)
{
	struct task_group *ptg = parent->tg;
	struct task_group *tg;
	double mult = parent->h_weight / parent->total_load;

	list_for_each_entry(tg, &ptg->children, siblings) {
		struct lb_group *g = linsched_tg(tg)->temp;
		if (g) {
			g->h_weight = tg->shares * mult;
			split_h_weight(g);
		}
	}
}

static void compute_lb_h_weight(void)
{
	int i;

	for(i = 0; i < n_lb_tasks; i++) {
		struct task_struct *p = lb_tasks[i].p;
		struct lb_group *g = linsched_tg(task_group(p))->temp;
		g->total_load += p->se.load.weight;
	}

	for(i = 0; i < n_lb_groups; i++) {
		struct task_group *tg = lb_groups[i].tg;
		struct lb_group *g;
		if (!tg->parent)
			continue;
		g = linsched_tg(tg->parent)->temp;
		g->total_load += tg->shares;
	}

	lb_groups[0].h_weight = 1024;

	split_h_weight(&lb_groups[0]);

	for(i = 0; i < n_lb_tasks; i++) {
		struct task_struct *p = lb_tasks[i].p;
		struct lb_group *g = linsched_tg(task_group(p))->temp;
		double mult = g->h_weight / g->total_load;
		lb_tasks[i].h_weight = p->se.load.weight * mult;
	}
}


#define num_compare(a, b) ((a) < (b) ? -1 : ((a) > (b) ? 1 : 0))

static int cpu_load_compare(const struct lb_cpu *a, const struct lb_cpu *b)
{
	return num_compare(a->load, b->load);
}

/* sort lb_task by increasing cpus_allowed, decreasing weight */
static int task_compare(const struct lb_task *a, const struct lb_task *b)
{
	int res;
	res = num_compare(a->cpus_allowed, b->cpus_allowed);
	if (!res)
		res = -num_compare(a->h_weight, b->h_weight);
	return res;
}

static void simple_greedy_balance(struct lb_cpu *cpus, int nr_cpus)
{
	int i;

	qsort_tasks(lb_tasks, n_lb_tasks);

	for(i = 0; i < n_lb_tasks; i++) {
		struct lb_cpu *j, *end;
		for(j = cpus; j < cpus + nr_cpus; j++) {
			if(cpumask_test_cpu(j->id,
					    &lb_tasks[i].p->cpus_allowed))
				break;
		}
		j->load += lb_tasks[i].h_weight;

		/* sort it into place */
		end = &cpus[nr_cpus - 1];
		for(; j < end && j->load > (j + 1)->load; j++) {
			struct lb_cpu temp = *j;
			*j = *(j + 1);
			*(j + 1) = temp;
		}
	}
}

static void actual_balance(struct lb_cpu *cpus, int nr_cpus)
{
	int i, cpu;
	struct lb_cpu *cpu_map[NR_CPUS]; /* in case the online set is not 0-n */

	for(i = 0; i < nr_cpus; i++) {
		cpu_map[cpus[i].id] = &cpus[i];
	}
	for(i = 0; i < n_lb_tasks; i++) {
		cpu = task_cpu(lb_tasks[i].p);
		cpu_map[cpu]->load += lb_tasks[i].h_weight;
	}
	qsort_cpus(cpus, nr_cpus);
}

void init_lb_info(void)
{
	start_time = 0;
	total_imbalance = 0;
}

hash_t mixhash(hash_t hash, uintptr_t value) {
	/* better hash functions could be used here, but this is
	 * empirically good enough and is noticeably faster */
	return hash * 29 + value;
}

static hash_t setup_lb_info(void)
{
	int i, out, cpu;
	struct task_struct *p;
	extern int num_tasks, num_cgroups; /* defined in linux_linsched.c */
	hash_t hash = 0;

	out = 0;
	for(i = 1; i < num_tasks; i++) {
		int j;
		p = __linsched_tasks[i];
		if (!p->se.on_rq)
			continue;
		lb_tasks[out].p = p;
		lb_tasks[out].cpus_allowed = cpumask_weight(&p->cpus_allowed);

		hash = mixhash(hash, (uintptr_t)p);
		hash = mixhash(hash, (uintptr_t)p->se.cfs_rq);
		hash = mixhash(hash, p->se.load.weight);

		/* hash in cpus_allowed */
		for (j = 0; j < BITS_TO_LONGS(NR_CPUS); j++) {
			hash = mixhash(hash, cpumask_bits(&p->cpus_allowed)[j]);
		}

		out++;
	}
	n_lb_tasks = out;
	out = 0;
	for(i = 0; i < num_cgroups; i++) {
		struct task_group *tg = cgroup_tg(&__linsched_cgroups[i].cg);
		int on_rq = 0;
		if (!tg)
			continue;
		if (tg != &root_task_group) {
			for_each_online_cpu(cpu) {
				if (tg->se[cpu]->on_rq) {
					on_rq = 1;
					break;
				}
			}
			if (!on_rq) {
				linsched_tg(tg)->temp = NULL;
				continue;
			}
		}
		lb_groups[out].tg = tg;
		lb_groups[out].total_load = 0;
		linsched_tg(tg)->temp = &lb_groups[out];
		hash = mixhash(hash, (uintptr_t)tg);
		hash = mixhash(hash, tg->shares);
		out++;
	}
	n_lb_groups = out;
	return hash;
}

static double compute_imbalance(void)
{
	int cpu, i = 0;
	int nr_cpus = num_online_cpus();
	static struct lb_cpu greedy_cpus[NR_CPUS];
	static struct lb_cpu actual_cpus[NR_CPUS];
	double imbalance = 0;

	compute_lb_h_weight();

	for_each_online_cpu(cpu) {
		greedy_cpus[i].load = 0;
		greedy_cpus[i].id = cpu;
		actual_cpus[i].load = 0;
		actual_cpus[i].id = cpu;
		i++;
	}
	simple_greedy_balance(greedy_cpus, nr_cpus);
	actual_balance(actual_cpus, nr_cpus);
	if (actual_cpus[nr_cpus - 1].load - actual_cpus[0].load <
	    greedy_cpus[nr_cpus - 1].load - greedy_cpus[0].load) {
		return NAN;
	}

	for(i = 0; i < nr_cpus; i++) {
		double delta = greedy_cpus[i].load - actual_cpus[i].load;
		imbalance += delta * delta;
	}
	return imbalance;
}

void compute_lb_info(void)
{
	double imbalance;
	u64 old_time = last_time;
	hash_t hash_key;

	last_time = current_time;
	if (!start_time) {
		start_time = current_time;
		return;
	}

	hash_key = setup_lb_info();

	struct hash_bucket *b = &hash_table[hash_key % HASH_TABLE_SIZE];
	if (b->key == hash_key) {
		imbalance = b->value;
	} else {
		imbalance = compute_imbalance();

		b->key = hash_key;
		b->value = imbalance;
	}

	if (isnan(imbalance)) {
		printf("actual balance is better than greedy balance at %llu\n", current_time);
		return;
	}

	total_imbalance += imbalance * (current_time - old_time);

	if (linsched_global_options.dump_imbalance) {
		printf("imbalance at %llu: %f\n",
		       current_time, imbalance);
	}
	if (linsched_global_options.dump_full_balance) {
		dump_lb_info(stdout);
	}
}

double get_average_imbalance(void)
{
	return total_imbalance / (current_time - start_time);
}

void dump_lb_info(FILE *out)
{
	int i;
	int cpu;
	char path[128];

	fprintf(out, "at %llu\n", current_time);
	for(i = 0; i < LINSCHED_MAX_TASKS; i++) {
		struct task_struct *p = __linsched_tasks[i];
		char mask[128];
		if (!p)
			continue;

		cgroup_path(task_group(p)->css.cgroup, path, sizeof(path));
		cpumask_scnprintf(mask, sizeof(mask), &p->cpus_allowed);
		fprintf(out, "task %d in %s w %lu cpu %d mask %s on_rq %d\n", i,
			path, p->se.load.weight, task_cpu(p),
			mask, p->se.on_rq);
	}
	for(i = 0; i < LINSCHED_MAX_GROUPS; i++) {
		struct cgroup *cgroup = &__linsched_cgroups[i].cg;
		struct task_group *tg = cgroup_tg(cgroup);
		if (!tg)
			continue;
		cgroup_path(cgroup, path, sizeof(path));

		fprintf(out, "group %s w %lu", path, tg->shares);

		if (tg->parent) {
			fprintf(out, " percpu:");
			for_each_online_cpu(cpu) {
				fprintf(out, " %lu", tg->se[cpu]->load.weight);
			}
		}
		fprintf(out, "\n");

	}

	for_each_online_cpu(cpu) {
		struct rb_node *cur;
		struct cfs_rq *cfs = root_task_group.cfs_rq[cpu];
		fprintf(out, "cpu %d %lu/%lu entities:", cpu, cfs->load.weight, cfs->nr_running);

		if (cfs->curr)
			fprintf(out, " %lu", cfs->curr->load.weight);

		cur = cfs->rb_leftmost;
		while(cur) {
			fprintf(out, " %lu", rb_entry(cur, struct sched_entity, run_node)->load.weight);
			cur = rb_next(cur);
		}
		fprintf(out, "\n");

	}
	fprintf(out, "\n");
}
