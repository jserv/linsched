/* LinSched -- The Linux Scheduler Simulator
 * Copyright (C) 2008  John M. Calandrino
 * E-mail: jmc@cs.unc.edu
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

#ifndef LINUX_LINSCHED_H
#define LINUX_LINSCHED_H

/* Additional functions provided to kernel-land */
#include <linux/kernel.h>

void linsched_change_cpu(int cpu);
void *malloc(size_t size);
void free(void *p);

#define linsched_alloc_thread_info(tsk) ((struct thread_info *) \
		malloc(sizeof(struct thread_info) + 1))
#define linsched_alloc_task_struct() ((struct task_struct *) \
		malloc(sizeof(struct task_struct) + 1))

#endif /* LINUX_LINSCHED_H */
