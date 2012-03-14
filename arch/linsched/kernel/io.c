#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/iocontext.h>

struct io_context *alloc_io_context(gfp_t gfp_flags, int node)
{
	return kzalloc(sizeof(struct io_context), gfp_flags);
}

void exit_io_context(struct task_struct *p)
{
}

struct io_context *get_task_io_context(struct task_struct *task,
					gfp_t gfp_flags, int node)
{
	BUG();
	return NULL;
}

void put_io_context(struct io_context *ioc)
{
	BUG();
}
