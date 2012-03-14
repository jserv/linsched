#ifndef __LINSCHED_PTRACE_H
#define __LINSCHED_PTRACE_H

struct pt_regs {
	unsigned long cs;
	unsigned long pc;
	unsigned long fp;
	unsigned long usp;
};

static inline int user_mode(struct pt_regs *regs)
{
	return 0;
}

#include <asm-generic/ptrace.h>

#endif
