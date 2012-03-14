#ifndef _ASM_LINSCHED_SPINLOCK_H
#define _ASM_LINSCHED_SPINLOCK_H

#include <asm/rwlock.h>

static inline int arch_spin_is_locked(arch_spinlock_t *lock)
{
	return lock->slock;
}

static inline int arch_spin_is_contended(arch_spinlock_t *lock)
{
	return 0;
}

static __always_inline void arch_spin_lock(arch_spinlock_t *lock)
{
	BUG_ON(lock->slock);
	lock->slock = 1;
}

static __always_inline int arch_spin_trylock(arch_spinlock_t *lock)
{
	/* should never be contended */
	arch_spin_lock(lock);
	return 1;
}

static __always_inline void arch_spin_unlock(arch_spinlock_t *lock)
{
	BUG_ON(!lock->slock);
	lock->slock = 0;
}

static __always_inline void arch_spin_lock_flags(arch_spinlock_t *lock,
						  unsigned long flags)
{
	arch_spin_lock(lock);
}

static inline void arch_spin_unlock_wait(arch_spinlock_t *lock)
{
}

/*
 * Read-write spinlocks, allowing multiple readers
 * but only one writer.
 *
 * NOTE! it is quite common to have readers in interrupts
 * but no interrupt writers. For those circumstances we
 * can "mix" irq-safe locks - any writer needs to get a
 * irq-safe write-lock, but readers can get non-irqsafe
 * read-locks.
 *
 * On x86, we implement read-write locks as a 32-bit counter
 * with the high bit (sign) being the "contended" bit.
 */

/**
 * read_can_lock - would read_trylock() succeed?
 * @lock: the rwlock in question.
 */
static inline int arch_read_can_lock(arch_rwlock_t *lock)
{
	return (int)(lock)->lock > 0;
}

/**
 * write_can_lock - would write_trylock() succeed?
 * @lock: the rwlock in question.
 */
static inline int arch_write_can_lock(arch_rwlock_t *lock)
{
	return (lock)->lock == RW_LOCK_BIAS;
}

/* for rw locks we use < RW_LOCK_BIAS for readers, > RW_LOCK_BIAS for writers */
static inline void arch_read_lock(arch_rwlock_t *rw)
{
	BUG_ON(--rw->lock >= RW_LOCK_BIAS);
}

static inline void arch_write_lock(arch_rwlock_t *rw)
{
	BUG_ON(++rw->lock != RW_LOCK_BIAS + 1);
}

static inline int arch_read_trylock(arch_rwlock_t *lock)
{
	if (lock->lock > 0)
		return 0;

	arch_read_lock(lock);
	return 1;
}

static inline int arch_write_trylock(arch_rwlock_t *lock)
{
	if (lock->lock != 0)
		return 0;

	arch_write_lock(lock);
	return 1;
}

static inline void arch_read_unlock(arch_rwlock_t *rw)
{
	rw->lock++;
}

static inline void arch_write_unlock(arch_rwlock_t *rw)
{
	rw->lock--;
}

#define arch_read_lock_flags(lock, flags) arch_read_lock(lock)
#define arch_write_lock_flags(lock, flags) arch_write_lock(lock)

#define arch_spin_relax(lock)	cpu_relax()
#define arch_read_relax(lock)	cpu_relax()
#define arch_write_relax(lock)	cpu_relax()

#define ARCH_HAS_SMP_MB_AFTER_LOCK

#endif /* _ASM_LINSCHED_SPINLOCK_H */
