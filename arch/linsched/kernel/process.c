#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/tty.h>

struct task_struct;

/*
 * This gets called before we allocate a new thread and copy
 * the current task into it.
 */
void prepare_to_copy(struct task_struct *tsk)
{
}

void show_regs(struct pt_regs *regs)
{
}

void arch_task_cache_init(void)
{
	BUG();
}

void flush_sigqueue(struct sigpending *queue)
{
	BUG();
}

void tty_kref_put(struct tty_struct *tty)
{
}

void release_thread(struct task_struct *dead_task)
{
}

bool do_notify_parent(struct task_struct *tsk, int sig)
{
	BUG();
	return true;
}

void __ptrace_unlink(struct task_struct *child)
{
	BUG();
}

void change_pid(struct task_struct *task, enum pid_type type,
			struct pid *pid)
{
	BUG();
}

struct thread_info *current_thread_info(void)
{
	return (struct thread_info *)current->stack;
}
