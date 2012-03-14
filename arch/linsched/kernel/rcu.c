#include <linux/sched.h>

int rcu_needs_cpu(int cpu)
{
	return 0;
}

void rcu_enter_nohz(void)
{
}

int init_srcu_struct(struct srcu_struct *sp)
{
	BUG();
	return 0;
}

int __srcu_read_lock(struct srcu_struct *sp)
{
	BUG();
	/*
	 * returning a magic number for now.
	 */
	return 1;
}

void __srcu_read_unlock(struct srcu_struct *sp, int idx)
{
	BUG();
}

void synchronize_srcu(struct srcu_struct *sp)
{
	BUG();
}

void rcu_check_callbacks(int cpu, int user)
{
}

void rcu_sched_qs(int cpu) {}

void rcu_exit_nohz(void)
{
}

void synchronize_sched(void)
{
}
