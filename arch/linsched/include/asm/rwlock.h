#ifndef _ASM_LINSCHED_RWLOCK_H
#define _ASM_LINSCHED_RWLOCK_H

typedef union {
	s32 lock;
	s32 write;
} arch_rwlock_t;

#define RW_LOCK_BIAS		0x00100000

#define __ARCH_RW_LOCK_UNLOCKED		{ RW_LOCK_BIAS }

/* Actual code is in asm/spinlock.h or in arch/x86/lib/rwlock.S */

#endif /* _ASM_LINSCHED_RWLOCK_H */
