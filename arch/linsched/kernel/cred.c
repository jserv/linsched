#include <linux/cred.h>
#include <linux/sched.h>


/*
 * The initial credentials for the initial task
 */
struct cred init_cred = {
	.usage			= ATOMIC_INIT(4),
#ifdef CONFIG_DEBUG_CREDENTIALS
	.subscribers		= ATOMIC_INIT(2),
	.magic			= CRED_MAGIC,
#endif
	.cap_inheritable	= CAP_EMPTY_SET,
	.cap_permitted		= CAP_FULL_SET,
	.cap_effective		= CAP_FULL_SET,
	.cap_bset		= CAP_FULL_SET,
	.user			= INIT_USER,
	.user_ns		= &init_user_ns,
	.group_info		= &init_groups,
#ifdef CONFIG_KEYS
	.tgcred			= &init_tgcred,
#endif
};


int copy_creds(struct task_struct *p, unsigned long clone_flags){
	atomic_inc(&p->cred->user->__count);
	atomic_inc(&p->cred->user->processes);
	get_group_info(p->cred->group_info);
	return 0;
}
