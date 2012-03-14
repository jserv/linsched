#ifndef _ASM_LINSCHED_MMU_CONTEXT_H
#define _ASM_LINSCHED_MMU_CONTEXT_H

#include <asm/boot.h>

static inline void paravirt_activate_mm(struct mm_struct *prev,
					struct mm_struct *next)
{
}

/*
 * Used for LDT copy/destruction.
 */
int init_new_context(struct task_struct *tsk, struct mm_struct *mm);
void destroy_context(struct mm_struct *mm);


static inline void enter_lazy_tlb(struct mm_struct *mm, struct task_struct *tsk)
{
}

static inline void switch_mm(struct mm_struct *prev, struct mm_struct *next,
			     struct task_struct *tsk)
{
}

#define activate_mm(prev, next)			\
do {						\
	paravirt_activate_mm((prev), (next));	\
	switch_mm((prev), (next), NULL);	\
} while (0);

#define deactivate_mm(tsk, mm)			\
do {						\
} while (0)

#endif /* _ASM_LINSCHED_MMU_CONTEXT_H */
