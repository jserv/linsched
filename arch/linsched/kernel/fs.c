#include <linux/slab.h>
#include <linux/fs_struct.h>
#include <linux/fdtable.h>

void free_fs_struct(struct fs_struct *fs)
{
	BUG();
}

struct fs_struct *copy_fs_struct(struct fs_struct *old)
{
	return kzalloc(sizeof(struct fs_struct), GFP_KERNEL);
}

struct files_struct *dup_fd(struct files_struct *oldf, int *errorp)
{
	return kzalloc(sizeof(struct files_struct), GFP_KERNEL);
}

void exit_fs(struct task_struct *p)
{
}

int filp_close(struct file *filp, fl_owner_t id)
{
	BUG();
	return 0;
}

void daemonize_fs_struct(void)
{
	BUG();
}
