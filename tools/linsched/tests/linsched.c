/* LinSched -- The Linux Scheduler Simulator
 * Copyright (C) 2008  John M. Calandrino
 * E-mail: jmc@cs.unc.edu
 *
 * Example scheduling simulation. Tasks are created and then the simulation
 * is run for some number of ticks.
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

#define LINSCHED_TICKS 60000

int linsched_test_main(int argc, char **argv)
{
	struct task_struct *p;
	struct cgroup *cgrp;


	/* Initialize linsched. */
	linsched_init(NULL);

	cgrp = linsched_create_cgroup(root_cgroup, "test");
	sched_group_set_shares(cgroup_tg(cgrp), 2048);

	/* Create some tasks with "callbacks" that should be called
	 * every scheduling decision.
	 */
	linsched_create_normal_task(linsched_create_sleep_run(8, 2), 0);

	linsched_create_RTrr_task(linsched_create_sleep_run(8, 2), 90);
	linsched_create_RTfifo_task(linsched_create_sleep_run(8, 2), 55);

	/* create more tasks here... */

	p = linsched_create_normal_task(linsched_create_sleep_run(8, 2), 0);
	linsched_add_task_to_group(p, cgrp);

	linsched_create_normal_task(linsched_create_sleep_run(8, 2), 0);
	linsched_create_normal_task(linsched_create_sleep_run(8, 2), 0);
	linsched_create_normal_task(linsched_create_sleep_run(8, 2), 0);
	linsched_create_normal_task(linsched_create_sleep_run(8, 2), 0);

	/* Run simulation for 500 ticks. */
	linsched_run_sim(500);

#if 0
	/* Force migrations between two CPUs, and allow migrations
	 * afterwards (so load balancing will kick in eventually).
	 */
	linsched_force_migration(linsched_get_task(2), 0, 1);

#endif
	/* Run simulation to completion. */
	linsched_run_sim(LINSCHED_TICKS-500);

	return 0;
}
