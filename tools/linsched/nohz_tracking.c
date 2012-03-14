/* Tracking cpu nohz residency */

#include "linsched.h"
#include <stdio.h>

#define MAX_DOMAINS 10

struct nohz_data {
	u64 nohz_time;
	u64 last_change;
	int num_nohz_cpus;
};

/*
 * nohz_data[cpu][0] is the data for the individual cpu, and then it
 * goes up the sched_domain tree for 1..
 */
static struct nohz_data nohz_data[NR_CPUS][MAX_DOMAINS];
struct nohz;
extern struct nohz nohz;

static void do_track_nohz_sd_residency(int cpu, int level, int nr_cpus,
				       int cpu_delta)
{
	struct nohz_data *data = &nohz_data[cpu][level];
	BUG_ON(level >= MAX_DOMAINS);

	if (data->num_nohz_cpus == nr_cpus) {
		data->nohz_time += current_time - data->last_change;
	}
	data->last_change = current_time;
	data->num_nohz_cpus += cpu_delta;
}

void track_nohz_residency(int cpu)
{
	struct sched_domain *sd;
	int cpu_delta;
	int nohz_active = !cpu_online(cpu) ||
		!!cpumask_test_cpu(cpu, nohz.idle_cpus_mask);
	int i = 1;

	cpu_delta = nohz_active - nohz_data[cpu][0].num_nohz_cpus;
	do_track_nohz_sd_residency(cpu, 0, 1, cpu_delta);

	for_each_domain(cpu, sd) {
		do_track_nohz_sd_residency(cpumask_first(sched_domain_span(sd)),
					   i, sd->span_weight, cpu_delta);
		i++;
	}
}

static void print_cpu(int cpu)
{
	int i;
	struct sched_domain *sd;

	track_nohz_residency(cpu);
	printf("cpu%3d: %7.4f%% ", cpu, nohz_data[cpu][0].nohz_time * 100.0 / current_time);
	i = 1;
	for_each_domain(cpu, sd) {
		if (cpu != cpumask_first(sched_domain_span(sd)))
			break;
		printf("%7.4f%% ",
		       nohz_data[cpu][i].nohz_time * 100.0 / current_time);
		i++;
	}
	printf("\n");
}

void print_nohz_residency(void)
{
	struct cpumask to_print;
	struct sched_domain *sd;
	int cpu;

	printf("Time spent with tick disabled over %llu ms:\n",
	       current_time / NSEC_PER_MSEC);
	cpumask_copy(&to_print, cpu_possible_mask);
	cpu = cpumask_first(&to_print);

	/* headings */
	printf("%7s ", "level:");
	for_each_domain(cpu, sd) {
		printf("%8s ", sd->name);
	}
	printf("%8s ", "SYSTEM");
	printf("\n");

	/* print the nohz info in sched_domain hierarchy order */
	while(cpu < nr_cpu_ids) {
		print_cpu(cpu);
		cpumask_clear_cpu(cpu, &to_print);

		if (nr_cpu_ids == 1)
			break;

		sd = cpu_rq(cpu)->sd;
		while(sd) {
			cpu = cpumask_first_and(sched_domain_span(sd),
						&to_print);
			if (cpu < nr_cpu_ids)
				break;
			sd = sd->parent;
		}
	}

	if (!cpumask_empty(&to_print)) {
		printf("currently offline cpus:\n");
		while(!cpumask_empty(&to_print)) {
			cpu = cpumask_first(&to_print);
			print_cpu(cpu);
			cpumask_clear_cpu(cpu, &to_print);
		}
	}
}
