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
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/init_task.h>
#include <linux/fs.h>
#include <linux/mqueue.h>
#include <linux/personality.h>
#include <linux/fdtable.h>
#include <linux/fs_struct.h>

#include <asm/uaccess.h>
#include <asm/pgtable.h>

#include <malloc.h>
void abort(void);

int __linsched_curr_cpu = 0;
struct group_info init_groups = {.usage = ATOMIC_INIT(2) };

DEFINE_PER_CPU(struct task_struct *, current_task) = &init_task;

struct user_namespace init_user_ns = {
	.kref = {
		.refcount	= ATOMIC_INIT(3),
	},
	.creator = &root_user,
};

struct percpu_counter vm_committed_as;

/* needs correct initialization depending on core speeds */
unsigned int cpu_khz;
struct task_struct *kthreadd_task;

struct nsproxy init_nsproxy;
struct user_struct root_user;
struct exec_domain default_exec_domain;
struct fs_struct init_fs;
struct files_struct init_files;

static int printk_level = 5; /* only show KERN_NOTICE and up */

int tsk_fork_get_node(struct task_struct *tsk)
{
	return numa_node_id();
}

void timerfd_clock_was_set(void) {}

long do_no_restart_syscall(struct restart_block *param)
{
	return -EINTR;
}

int security_task_setnice(struct task_struct *p, int nice)
{
	return 0;
}

int __printk_ratelimit(const char *func)
{
	return 0;
}

void enter_idle(void)
{
}

void __exit_idle(void)
{
}

void exit_idle(void)
{
}

void linsched_set_printk_level(int level)
{
	printk_level = level;
}

asmlinkage int printk(const char *fmt, ...)
{
	va_list args;
	int log_level = 5; /* default to KERN_NOTICE */

	/* very basic log_prefix handling */
	if (fmt[0] == '<' && (fmt[1] >= '0' && fmt[1] <= '9') &&
	    fmt[2] == '>') {
		log_level = fmt[1] - '0';
	}
	if (log_level > printk_level)
		return 0;

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	return 0;
}

void panic(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	puts("");
	abort();
}

void dump_stack(void)
{
}

struct sighand_struct *__lock_task_sighand(struct task_struct *tsk,
					 unsigned long *flags)
{
	return tsk->sighand;
}

void linsched_change_cpu(int cpu)
{
	__linsched_curr_cpu = cpu;
}

unsigned int debug_smp_processor_id(void)
{
	return __linsched_curr_cpu;
}

bool capable(int cap)
{
	return 1;
}

/* These functions do not copy to and from user space anymore, so
 * they are just memory copy functions now.
 */
__must_check unsigned long
_copy_from_user(void *to, const void __user * from, unsigned n)
{
	memcpy(to, from, n);
	return 0;
}

__must_check unsigned long
_copy_to_user(void __user *to, const void *from, unsigned n)
{
	memcpy(to, from, n);
	return 0;
}

void copy_from_user_overflow(void)
{
}

void user_disable_single_step(struct task_struct *child)
{
}

void warn_slowpath_null(const char *file, int line)
{
	printf("WARNING: at %s:%d\n", file, line);
}

void warn_slowpath_fmt(const char *file, int line, const char *fmt, ...)
{
	va_list list;
	warn_slowpath_null(file, line);
	va_start(list, fmt);
	vprintf(fmt, list);
	va_end(list);
	puts("");
}

struct dentry *debugfs_create_file(const char *name, mode_t mode,
				struct dentry *parent, void *data,
				const struct file_operations *fops)
{
	return NULL;
}

ssize_t seq_read(struct file *file, char *buf, size_t size, loff_t *ppos)
{
	return 0;
}

loff_t seq_lseek(struct file *file, loff_t offset, int origin)
{
	return 0;
}

int single_release(struct inode *inode, struct file *file)
{
	return 0;
}

int seq_printf(struct seq_file *m, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	return 0;
}

int seq_puts(struct seq_file *m, const char *s)
{
	puts(s);

	return 0;
}

int single_open(struct file *file, int (*show)(struct seq_file *, void *),
		void *data)
{
	return 0;
}

void kprobe_flush_task(struct task_struct *tk)
{
}

void print_modules(void)
{
}
