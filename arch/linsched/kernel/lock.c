#include <linux/sched.h>

void rt_mutex_adjust_pi(struct task_struct *task)
{
}

int rt_mutex_getprio(struct task_struct *task)
{
	return task->normal_prio;
}
