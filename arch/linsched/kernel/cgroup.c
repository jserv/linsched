/* cgroup stubs */

#include <linux/cgroup.h>
#include <linux/fs.h>
#include <linux/flex_array.h>
#include <linux/sched.h>

#ifdef CONFIG_CGROUPS

static struct css_set init_css_set;

extern struct cgroup *root_cgroup;

/*
 * Trying to simulate cgroups as much as possible.
 * TODO: Check if really needed and actually remove the
 * variable
 */
static int need_forkexit_callback;

int cgroup_add_files(struct cgroup *cgrp, struct cgroup_subsys *subsys,
			const struct cftype cft[], int count)
{
	return 0;
}

void cgroup_fork(struct task_struct *child)
{
	task_lock(current);
	child->cgroups = malloc(sizeof *child->cgroups);
	*child->cgroups = *current->cgroups;
	task_unlock(current);
	INIT_LIST_HEAD(&child->cg_list);
}

#define SUBSYS(_x) &_x ## _subsys,
static struct cgroup_subsys *subsys[CGROUP_SUBSYS_COUNT] = {
#include <linux/cgroup_subsys.h>
};

static void init_cgroup_css(struct cgroup_subsys_state *css,
			       struct cgroup_subsys *ss,
			       struct cgroup *cgrp)
{
	css->cgroup = cgrp;
	atomic_set(&css->refcnt, 1);
	css->id = NULL;
	BUG_ON(cgrp->subsys[ss->subsys_id]);
	cgrp->subsys[ss->subsys_id] = css;
}

static void __init cgroup_init_subsys(struct cgroup_subsys *ss)
{
	struct cgroup_subsys_state *css;

	printk(KERN_INFO "Initializing cgroup subsys %s\n", ss->name);
	css = ss->create(ss, root_cgroup);
	init_cgroup_css(css, ss, root_cgroup);
	init_css_set.subsys[ss->subsys_id] = root_cgroup->subsys[ss->subsys_id];
	need_forkexit_callback |= ss->fork || ss->exit;
}

int __init cgroup_init_early(void)
{
	int i;
	atomic_set(&init_css_set.refcount, 1);
	INIT_LIST_HEAD(&init_css_set.cg_links);
	INIT_LIST_HEAD(&init_css_set.tasks);
	INIT_HLIST_NODE(&init_css_set.hlist);

	init_task.cgroups = &init_css_set;
	/* at bootup time, we don't worry about modular subsystems */
	for (i = 0; i < CGROUP_BUILTIN_SUBSYS_COUNT; i++) {
		struct cgroup_subsys *ss = subsys[i];

		BUG_ON(!ss->name);
		BUG_ON(strlen(ss->name) > MAX_CGROUP_TYPE_NAMELEN);
		BUG_ON(!ss->create);
		BUG_ON(!ss->destroy);
		if (ss->subsys_id != i) {
			printk(KERN_ERR "cgroup: Subsys %s id == %d\n",
			       ss->name, ss->subsys_id);
			BUG();
		}

		if (ss->early_init)
			cgroup_init_subsys(ss);
	}
	return 0;
}

/**
 * cgroup_path - generate the path of a cgroup
 * @cgrp: the cgroup in question
 * @buf: the buffer to write the path into
 * @buflen: the length of the buffer
 *
 * Called with cgroup_mutex held or else with an RCU-protected cgroup
 * reference.  Writes path of cgroup into buf.  Returns 0 on success,
 * -errno on error.
 */
int cgroup_path(const struct cgroup *cgrp, char *buf, int buflen)
{
	char *start;
	struct dentry *dentry = cgrp->dentry;

	if (!dentry || cgrp == root_cgroup) {
		/*
		 * Inactive subsystems have no dentry for their root
		 * cgroup
		 */
		strcpy(buf, "/");
		return 0;
	}

	start = buf + buflen;

	*--start = '\0';
	for (;;) {
		int len = dentry->d_name.len;

		if ((start -= len) < buf)
			return -ENAMETOOLONG;
		memcpy(start, dentry->d_name.name, len);
		cgrp = cgrp->parent;
		if (!cgrp)
			break;

		dentry = rcu_dereference_check(cgrp->dentry,
					       cgroup_lock_is_held());
		if (!cgrp->parent)
			continue;
		if (--start < buf)
			return -ENAMETOOLONG;
		*start = '/';
	}
	memmove(buf, start, buf + buflen - start);
	return 0;
}

void cgroup_fork_callbacks(struct task_struct *child)
{
	if (need_forkexit_callback) {
		int i;

		for (i = 0; i < CGROUP_BUILTIN_SUBSYS_COUNT; i++) {
			struct cgroup_subsys *ss = subsys[i];
			if (ss->fork)
				ss->fork(ss, child);
		}
	}
}

void cgroup_post_fork(struct task_struct *child)
{
}

/*
 * Control Group taskset
 */
struct task_and_cgroup {
	struct task_struct      *task;
	struct cgroup           *cgrp;
};

struct cgroup_taskset {
	struct task_and_cgroup  single;
	struct flex_array       *tc_array;
	int                     tc_array_len;
	int                     idx;
	struct cgroup           *cur_cgrp;
};


void cgroup_exit(struct task_struct *tsk, int run_callbacks)
{
}

struct task_struct *cgroup_taskset_first(struct cgroup_taskset *tset)
{
	if (tset->tc_array) {
		tset->idx = 0;
		return cgroup_taskset_next(tset);
	} else {
		tset->cur_cgrp = tset->single.cgrp;
		return tset->single.task;
	}
}

struct task_struct *cgroup_taskset_next(struct cgroup_taskset *tset)
{
	struct task_and_cgroup *tc;

	if (!tset->tc_array || tset->idx >= tset->tc_array_len)
		return NULL;

	tc = flex_array_get(tset->tc_array, tset->idx++);
	tset->cur_cgrp = tc->cgrp;
	return tc->task;
}

struct cgroup *cgroup_taskset_cur_cgroup(struct cgroup_taskset *tset)
{
	return tset->cur_cgrp;
}

#endif
