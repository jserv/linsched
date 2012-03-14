#ifndef _ASM_LINSCHED_IRQ_REGS_H_
#define _ASM_LINSCHED_IRQ_REGS_H_

#include <asm/percpu.h>
#include <asm/ptrace.h>
#include <linux/slab.h>

#define ARCH_HAS_OWN_IRQ_REGS

static struct pt_regs irq_regs = {
	.cs	=	3,
};

static inline struct pt_regs *get_irq_regs(void)
{
	return &irq_regs;
}

static inline struct pt_regs *set_irq_regs(struct pt_regs *new_regs)
{
	BUG();
	return &irq_regs;
}

#endif
