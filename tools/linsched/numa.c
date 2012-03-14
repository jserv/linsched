/* LinSched -- The Linux Scheduler Simulator
 * Copyright (C) 2008  John M. Calandrino
 * E-mail: jmc@cs.unc.edu
 *
 * This file contains Linux variables and functions that have been "defined
 * away" or exist here in a modified form to avoid including an entire Linux
 * source file that might otherwise lead to a "cascade" of dependency issues.
 * It also includes certain LinSched variables to which some Linux functions
 * and definitions now map.
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

#include <linux/gfp.h>
#include <linux/hrtimer.h>

#include "linsched.h"

int nr_node_ids;
int nr_cpu_ids;
int cpu_to_node_map[NR_CPUS];
cpumask_var_t node_to_cpumask_map[MAX_NUMNODES];
static int cpu_to_coregroup_map[NR_CPUS];
static cpumask_var_t cpu_coregroup_masks[NR_CPUS];
static int cpu_to_core_map[NR_CPUS];
cpumask_var_t cpu_core_masks[NR_CPUS];
static int node_distances[MAX_NUMNODES][MAX_NUMNODES];
int smp_num_siblings = 1;

static struct hrtimer trigger_timer[NR_CPUS];

void linsched_init_cpus(struct linsched_topology *topo)
{
	int i;

	for (i = 0; i < NR_CPUS; i++) {
		zalloc_cpumask_var(&node_to_cpumask_map[i], GFP_KERNEL);
		zalloc_cpumask_var(&cpu_coregroup_masks[i], GFP_KERNEL);
		zalloc_cpumask_var(&cpu_core_masks[i], GFP_KERNEL);
	}

	if (topo) {
		memcpy(cpu_to_node_map, topo->node_map, sizeof(topo->node_map));
		memcpy(cpu_to_coregroup_map, topo->coregroup_map,
		       sizeof(topo->coregroup_map));
		memcpy(cpu_to_core_map, topo->core_map, sizeof(topo->core_map));
		memcpy(node_distances, topo->node_distances,
		       sizeof(topo->node_distances));

		nr_cpu_ids = topo->nr_cpus;
	} else {
		for (i = 0; i < LINSCHED_DEFAULT_NR_CPUS; i++) {
			cpu_to_core_map[i] = i;
			cpu_to_coregroup_map[i] = i;
		}
		/* all cpus go in node 0 */
		nr_cpu_ids = LINSCHED_DEFAULT_NR_CPUS;
	}

	for (i = 0; i < nr_cpu_ids; i++) {
		cpumask_set_cpu(i, node_to_cpumask_map[cpu_to_node_map[i]]);
		cpumask_set_cpu(i,
				cpu_coregroup_masks[cpu_to_coregroup_map[i]]);
		cpumask_set_cpu(i, cpu_core_masks[cpu_to_core_map[i]]);
		set_cpu_present(i, true);
		set_cpu_possible(i, true);
	}
	nr_node_ids = cpu_to_node_map[nr_cpu_ids - 1] + 1;
	smp_num_siblings = cpumask_weight(cpu_core_masks[0]);
}

const struct cpumask *cpu_coregroup_mask(int cpu)
{
	return cpu_coregroup_masks[cpu_to_coregroup_map[cpu]];
}

const struct cpumask *get_cpu_core_mask(int cpu)
{
	return cpu_core_masks[cpu_to_core_map[cpu]];
}

static enum hrtimer_restart cpu_triggered(struct hrtimer *t)
{
	scheduler_ipi();
	return HRTIMER_NORESTART;
}

void linsched_trigger_cpu(int cpu)
{
	int curr_cpu = smp_processor_id();
	linsched_change_cpu(cpu);
	if (!hrtimer_active(&trigger_timer[cpu])) {
		hrtimer_init(&trigger_timer[cpu], CLOCK_MONOTONIC,
			     HRTIMER_MODE_REL);
		hrtimer_set_expires(&trigger_timer[cpu], ktime_set(0, 1));
		trigger_timer[cpu].function = cpu_triggered;
		hrtimer_start(&trigger_timer[cpu], ktime_set(0, 1),
			      HRTIMER_MODE_REL);
	}
	/*
	 * Call the scheduler ipi when queueing up tasks on the wakelist
	 */
	scheduler_ipi();
	linsched_change_cpu(curr_cpu);
}

int __node_distance(int a, int b)
{
	return node_distances[a][b];
}

void __init smp_init(void)
{
	unsigned int cpu;

	for_each_present_cpu(cpu) {
		if (!cpu_online(cpu))
			cpu_up(cpu);
	}
}

void linsched_offline_cpu(int cpu)
{
	cpu_down(cpu);
}

void linsched_online_cpu(int cpu)
{
	cpu_up(cpu);
}
