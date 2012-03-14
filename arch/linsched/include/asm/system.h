#ifndef _ASM_LINSCHED_SYSTEM_H
#define _ASM_LINSCHED_SYSTEM_H

#include <linux/kernel.h>
#include <linux/irqflags.h>

struct task_struct; /* one of the stranger aspects of C forward declarations */
struct task_struct *__switch_to(struct task_struct *prev,
				struct task_struct *next);
struct tss_struct;
void __switch_to_xtra(struct task_struct *prev_p, struct task_struct *next_p,
		      struct tss_struct *tss);
extern void show_regs_common(void);

/*
 * Saving eflags is important. It switches not only IOPL between tasks,
 * it also protects other tasks from NT leaking through sysenter etc.
 */
#define switch_to(prev, next, last)					\
do {									\
	percpu_write(current_task, next);				\
} while (0)


#ifdef __KERNEL__
static unsigned long linsched_cr0, linsched_cr8, linsched_cr2, linsched_cr3, linsched_cr4;

static inline unsigned long native_read_cr0(void)
{
	unsigned long val;
	val = linsched_cr0;
	return val;
}

static inline void native_write_cr0(unsigned long val)
{
	linsched_cr0 = val;
}

static inline unsigned long native_read_cr2(void)
{
	unsigned long val;
	val = linsched_cr2;
	return val;
}

static inline void native_write_cr2(unsigned long val)
{
	linsched_cr2 = val;
}

static inline unsigned long native_read_cr3(void)
{
	unsigned long val;
	val = linsched_cr3;
	return val;
}

static inline void native_write_cr3(unsigned long val)
{
	linsched_cr3 = val;
}

static inline unsigned long native_read_cr4(void)
{
	unsigned long val;
	val = linsched_cr4;
	return val;
}

static inline unsigned long native_read_cr4_safe(void)
{
	unsigned long val;
	val = linsched_cr4;
	return val;
}

static inline void native_write_cr4(unsigned long val)
{
	linsched_cr4 = val;
}

static inline unsigned long native_read_cr8(void)
{
	unsigned long val;
	val = linsched_cr8;
	return val;
}

static inline void native_write_cr8(unsigned long val)
{
	linsched_cr8 = val;
}

static inline void native_wbinvd(void)
{
	asm volatile("wbinvd": : :"memory");
}

#define read_cr0()	(native_read_cr0())
#define write_cr0(x)	(native_write_cr0(x))
#define read_cr2()	(native_read_cr2())
#define write_cr2(x)	(native_write_cr2(x))
#define read_cr3()	(native_read_cr3())
#define write_cr3(x)	(native_write_cr3(x))
#define read_cr4()	(native_read_cr4())
#define read_cr4_safe()	(native_read_cr4_safe())
#define write_cr4(x)	(native_write_cr4(x))
#define wbinvd()	(native_wbinvd())
#define read_cr8()	(native_read_cr8())
#define write_cr8(x)	(native_write_cr8(x))
#define load_gs_index   native_load_gs_index

#define stts() write_cr0(read_cr0() | X86_CR0_TS)

#endif /* __KERNEL__ */

static inline void clflush(volatile void *__p)
{
	asm volatile("clflush %0" : "+m" (*(volatile char __force *)__p));
}

#define nop() asm volatile ("nop")

void disable_hlt(void);
void enable_hlt(void);

void cpu_idle_wait(void);

extern unsigned long arch_align_stack(unsigned long sp);
extern void free_init_pages(char *what, unsigned long begin, unsigned long end);

void default_idle(void);

void stop_this_cpu(void *dummy);

/*
 * Cannot do strict ordering in the simulator
 */
#define mb()
#define rmb()
#define wmb()

/**
 * read_barrier_depends - Flush all pending reads that subsequents reads
 * depend on.
 *
 * No data-dependent reads from memory-like regions are ever reordered
 * over this barrier.  All reads preceding this primitive are guaranteed
 * to access memory (but not necessarily other CPUs' caches) before any
 * reads following this primitive that depend on the data return by
 * any of the preceding reads.  This primitive is much lighter weight than
 * rmb() on most CPUs, and is never heavier weight than is
 * rmb().
 *
 * These ordering constraints are respected by both the local CPU
 * and the compiler.
 *
 * Ordering is not guaranteed by anything other than these primitives,
 * not even by data dependencies.  See the documentation for
 * memory_barrier() for examples and URLs to more information.
 *
 * For example, the following code would force ordering (the initial
 * value of "a" is zero, "b" is one, and "p" is "&a"):
 *
 * <programlisting>
 *	CPU 0				CPU 1
 *
 *	b = 2;
 *	memory_barrier();
 *	p = &b;				q = p;
 *					read_barrier_depends();
 *					d = *q;
 * </programlisting>
 *
 * because the read of "*q" depends on the read of "p" and these
 * two reads are separated by a read_barrier_depends().  However,
 * the following code, with the same initial values for "a" and "b":
 *
 * <programlisting>
 *	CPU 0				CPU 1
 *
 *	a = 2;
 *	memory_barrier();
 *	b = 3;				y = b;
 *					read_barrier_depends();
 *					x = a;
 * </programlisting>
 *
 * does not enforce ordering, since there is no data dependency between
 * the read of "a" and the read of "b".  Therefore, on some CPUs, such
 * as Alpha, "y" could be set to 3 and "x" to 0.  Use rmb()
 * in cases like this where there are no data dependencies.
 **/

#define read_barrier_depends()	do { } while (0)

#define smp_mb()
#define smp_rmb()
#define smp_wmb()
#define smp_read_barrier_depends()
#define set_mb(var, value) do { var = value;} while (0)

#endif /* _ASM_LINSCHED_SYSTEM_H */
