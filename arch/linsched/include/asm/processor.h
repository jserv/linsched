#ifndef __LINSCHED_PROCESSOR_H_
#define __LINSCHED_PROCESSOR_H_

#include <asm/pgtable_types.h>

#define HBP_NUM 4

struct thread_struct {
	/* Cached TLS descriptors: */
	unsigned long		sp0;
	unsigned long		sp;
};

typedef struct {
	unsigned long           seg;
} mm_segment_t;


/* Free all resources held by a thread. */
extern void release_thread(struct task_struct *);

/* Prepare to copy thread state - unlazy all lazy state */
extern void prepare_to_copy(struct task_struct *tsk);

unsigned long get_wchan(struct task_struct *p);

/* REP NOP (PAUSE) is a good thing to insert into busy-wait loops. */
static inline void rep_nop(void)
{
	asm volatile("rep; nop" ::: "memory");
}

static inline void cpu_relax(void)
{
	rep_nop();
}

/*
 * User space process size. 47bits minus one guard page.
 */
#define TASK_SIZE_MAX	((1UL << 47) - PAGE_SIZE)

/* This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define IA32_PAGE_OFFSET	((current->personality & ADDR_LIMIT_3GB) ? \
					0xc0000000 : 0xFFFFe000)

#define TASK_SIZE		(test_thread_flag(TIF_IA32) ? \
					IA32_PAGE_OFFSET : TASK_SIZE_MAX)
#define TASK_SIZE_OF(child)	((test_tsk_thread_flag(child, TIF_IA32)) ? \
					IA32_PAGE_OFFSET : TASK_SIZE_MAX)

#define STACK_TOP		TASK_SIZE
#define STACK_TOP_MAX		TASK_SIZE_MAX

#define INIT_THREAD  { \
	.sp0 = (unsigned long)&init_stack + sizeof(init_stack) \
}

/*
 * Return saved PC of a blocked thread.
 * What is this good for? it will be always the scheduler or ret_from_fork.
 */
#define thread_saved_pc(t)	(*(unsigned long *)((t)->thread.sp - 8))

#define task_pt_regs(tsk)	((struct pt_regs *)(tsk)->thread.sp0 - 1)

/*
 * This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE	(PAGE_ALIGN(TASK_SIZE / 3))

#endif /* _ASM_LINSCHED_PROCESSOR_H */
