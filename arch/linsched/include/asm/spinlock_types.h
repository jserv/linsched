#ifndef __LINSCHED_SPINLOCK_TYPES_H
#define __LINSCHED_SPINLOCK_TYPES_H

#ifndef __LINSCHED_SPINLOCK_TYPES_H
#error "please don't include this file directly"
#endif

typedef struct {
	volatile unsigned int slock;
} arch_spinlock_t;

#define __ARCH_SPIN_LOCK_UNLOCKED	{ 0 }

#include <asm/rwlock.h>

#endif
