#ifndef __LINSCHED_IRQFLAGS_H_
#define __LINSCHED_IRQFLAGS_H_

extern unsigned long linsched_irq_flags;

static unsigned long linsched_save_flags(void)
{
	return linsched_irq_flags;
}

#define arch_local_save_flags	linsched_save_flags

static void linsched_restore_flags(unsigned long flags)
{
	linsched_irq_flags = flags;
}

#define arch_local_irq_restore(x)	linsched_restore_flags(x)

#include <asm-generic/irqflags.h>

#endif
