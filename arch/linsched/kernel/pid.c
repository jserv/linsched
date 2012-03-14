#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/pid_namespace.h>
#include <linux/slab.h>

/* TODO: this is rather.. inelegant */
static struct pid *pid_mapping[32000];

/*
 * We don't support namespaces so we just return a
 * pid to be used, which is basically next_pid
 */

static int alloc_pidmap(struct pid_namespace *pid_ns)
{
	static int next_pid = 2;

	return next_pid++;
}

/*
 * We don't support namespaces, so skip all the additional
 * handling there.
 */
struct pid *alloc_pid(struct pid_namespace *ns)
{
	struct pid *pid;
	int i = 0, nr;

	pid = kzalloc(sizeof(struct pid), GFP_KERNEL);
	if (!pid)
		return NULL;

	nr = alloc_pidmap(ns);
	if (nr < 0)
		goto out_free;

	pid->numbers[i].nr = nr;
	pid->numbers[i].ns = ns;

	pid_mapping[nr - 1] = pid;

	return pid;

out_free:
	kfree(pid);
	pid = NULL;
	return pid;
}

void free_pid(struct pid *pid)
{
	kfree(pid);
}

void put_pid(struct pid *pid)
{
	BUG();
}

struct pid *find_get_pid(pid_t nr)
{
	BUG_ON(!pid_mapping[nr - 1]);

	return pid_mapping[nr - 1];
}

struct task_struct *pid_task(struct pid *pid, enum pid_type type)
{
	struct task_struct *result = NULL;

	if (pid) {
		result = hlist_entry(pid->tasks[type].first, struct task_struct,
				pids[type].node);
	}

	return result;
}

struct task_struct *find_task_by_vpid(pid_t vnr)
{
	return pid_task(find_get_pid(vnr), PIDTYPE_PID);
}

struct pid *get_task_pid(struct task_struct *task, enum pid_type type)
{
	BUG();
	return NULL;
}

pid_t pid_vnr(struct pid *pid)
{
	BUG();
	return 0;
}

void detach_pid(struct task_struct *task, enum pid_type type)
{
	BUG();
}

/*
 * We don't have any sort of namespaces, input pid is output pid.
 */
pid_t __task_pid_nr_ns(struct task_struct *task, enum pid_type type,
			struct pid_namespace *ns)
{
	return task->pid;
}

struct pid_namespace *task_active_pid_ns(struct task_struct *task)
{
	return &init_pid_ns;
}
