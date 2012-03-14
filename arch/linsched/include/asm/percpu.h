#ifndef __ASM_LINSCHED_PERCPU_H
#define __ASM_LINSCHED_PERCPU_H

#include <asm-generic/percpu.h>

extern unsigned int debug_smp_processor_id(void);
#define raw_smp_processor_id() debug_smp_processor_id()

#define percpu_read_stable(var) __raw_get_cpu_var(var)

/*
 * Set up an array of pointers for debugging, and only ever have one section
 */
struct percpu_info {
	void **array;
	void *base;
};

#undef DECLARE_PER_CPU_SECTION  /* We undefine the percpu-defs version */
#define DECLARE_PER_CPU_SECTION(type, name, sec)			\
	extern __PCPU_ATTRS("") __typeof__(type) name


#undef DEFINE_PER_CPU_SECTION
#define DEFINE_PER_CPU_SECTION(type, name, sec)				\
	__attribute__((section(".data..percpuarrays"))) __typeof__(type) *__pcpu_##name[NR_CPUS]; \
	extern __PCPU_ATTRS("") PER_CPU_DEF_ATTRIBUTES __typeof__(type) name; \
	struct percpu_info __attribute__((section(".data..percpuinfo"))) \
		__per_cpu_info_##name = { (void**)&__pcpu_##name, &name }; \
	__PCPU_ATTRS("") PER_CPU_DEF_ATTRIBUTES __typeof__(type) name

#endif /* __ASM_LINSCHED_PERCPU_H */
