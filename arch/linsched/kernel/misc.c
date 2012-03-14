#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/fs_struct.h>
#include <linux/module.h>
#include <linux/nsproxy.h>
#include <linux/device.h>

#include <linux/clocksource.h>
#include <linux/syscore_ops.h>

#include <asm/irq_regs.h>

DEFINE_PER_CPU(int, dirty_throttle_leaks) = 0;

void exit_creds(struct task_struct *txk)
{
}

void prop_local_destroy_single(struct prop_local_single *pl)
{
}

int prop_local_init_single(struct prop_local_single *pl)
{
	return 0;
}

long sys_futex(u32 *uaddr, int op, u32 val, struct timespec *utime,
		u32 uaddr2, u32 val3)
{
       /* returning non-zero, but should test this */
	return 1;
}

void module_put(struct module *module)
{
}

int unshare_nsproxy_namespaces(unsigned long unshare_flags,
		struct nsproxy **new_nsp, struct fs_struct *new_fs)
{
	BUG();
	return 0;
}

void switch_task_namespaces(struct task_struct *p, struct nsproxy *new)
{
	BUG();
}

void free_nsproxy(struct nsproxy *ns)
{
	BUG();
}

int __init nsproxy_cache_init(void)
{
	return 0;
}

enum hrtimer_restart it_real_fn(struct hrtimer *time)
{
	return HRTIMER_NORESTART;
}

int copy_namespaces(unsigned long flags, struct task_struct *tsk)
{
	struct nsproxy *old_ns = tsk->nsproxy;
	struct nsproxy *new_ns;

	new_ns = kzalloc(sizeof(struct nsproxy), GFP_KERNEL);
	if (!new_ns)
		return -ENOMEM;

	atomic_set(&new_ns->count, 1);

	tsk->nsproxy = new_ns;

	return 0;
}

void exit_task_namespaces(struct task_struct *p)
{
}

int copy_thread(unsigned long clone_flags, unsigned long sp,
		unsigned long unused, struct task_struct *p,
		struct pt_regs *regs)
{
	return 0;
}

void recalc_sigpending(void)
{
}

void __ptrace_link(struct task_struct *child, struct task_struct *new_parent)
{
}

bool task_set_jobctl_pending(struct task_struct *task, unsigned int mask)
{
	return true;
}

void ptrace_notify(int exit_code)
{
}

int send_sig(int sig, struct task_struct *p, int priv)
{
	return 0;
}

void mod_zone_page_state(struct zone *zone, enum zone_stat_item item, int i)
{
}

int printk_needs_cpu(int cpu)
{
	return 0;
}

void vmalloc_sync_all(void)
{
	BUG();
}

/*
 * We don't support capabilities.
 */
int cap_settime(const struct timespec *ts, const struct timezone *tz)
{
	BUG();
	return 0;
}

void clocksource_resume(void)
{
	BUG();
}

void clocksource_suspend(void)
{
	BUG();
}

void register_syscore_ops(struct syscore_ops *ops)
{
	BUG();
}

void
clocks_calc_mult_shift(u32 *mult, u32 *shift, u32 from, u32 to, u32 maxsec)
{
	BUG();
}

int clocksource_register(struct clocksource *cs)
{
	BUG();
	return 0;
}

void free_fdtable_rcu(struct rcu_head *rcu)
{
	BUG();
}

int zap_other_threads(struct task_struct *p)
{
	BUG();
	return 0;
}

void proc_clear_tty(struct task_struct *p)
{
	BUG();
}

void flush_signals(struct task_struct *t)
{
	BUG();
}

int commit_creds(struct cred *new)
{
	BUG();
	return 0;
}

int __kill_pgrp_info(int sig, struct siginfo *info, struct pid *pgrp)
{
	return 0;
}

/*
 * Might need the lock annotations
 */
void exit_ptrace(struct task_struct *tracer)
{
}

int group_send_sig_info(int sig, struct siginfo *info, struct task_struct *p)
{
	return 0;
}

void exit_irq_thread(void)
{
}

void exit_signals(struct task_struct *tsk)
{
}

void acct_collect(long exitcode, int group_dead)
{
}

void exit_thread(void)
{
}

void __free_pipe_info(struct pipe_inode_info *pipe)
{
}

void exit_itimers(struct signal_struct *sig)
{
}

void acct_process(void)
{
}

void disassociate_ctty(int on_exit)
{
}

void printk_tick(void)
{
}

unsigned int alarm_setitimer(unsigned int seconds)
{
	BUG();
	return 0;
}

void si_meminfo(struct sysinfo *val)
{
	BUG();
}

void si_swapinfo(struct sysinfo *val)
{
	BUG();
}

/*
 * we don't support either namespaces or capability. So this test should
 * always return true.
 */
bool ns_capable(struct user_namespace *ns, int cap)
{
	return true;
}

bool ptrace_may_access(struct task_struct *task, unsigned int mode)
{
	BUG();
	return true;
}

void exit_robust_list(struct task_struct *curr)
{
}

void exit_pi_state_list(struct task_struct *curr)
{
}

void signalfd_cleanup(struct sighand_struct *sighand)
{
}

/*
 * We don't support sysfs, but need to check the consequences of this
 * decision.
 */
int device_create_file(struct device *dev,
		const struct device_attribute *attr)
{
	return 0;
}

/* kernel/dumpstack.c */
void show_stack(struct task_struct *task, unsigned long *sp)
{
	/* should be unused */
	BUG();
}

/* kernel/capability.c */
bool task_ns_capable(struct task_struct *t, int cap)
{
	return true;
}

/* kernel/blk-core.c */
void blk_flush_plug_list(struct blk_plug *plug, bool from_schedule)
{
	/* should be unused */
	BUG();
}

/* fs/sysfs/file.c */
int sysfs_create_file(struct kobject *kobj, const struct attribute * attr)
{
	return 0;
}

/* kernel/sysctl.c */
int proc_dointvec(struct ctl_table *table, int write,
		  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	/* should be unused */
	BUG();

	return 0;
}

int proc_dointvec_minmax(struct ctl_table *table, int write,
		    void __user *buffer, size_t *lenp, loff_t *ppos)
{
	/* should be unused */
	BUG();

	return 0;
}
