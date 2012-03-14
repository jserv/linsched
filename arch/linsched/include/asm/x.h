#if 0
#ifndef __LINSCHED_PERCPU_H
#define __LINSCHED_PERCPU_H

#define DECLARE_PER_CPU_SECTION(type, name, sec)			\
	extern __PCPU_ATTRS("") __typeof__(type) name


struct percpu_info {
	void **array;
	void *base;
};

/*
 * Set up an array of pointers for debugging, and only ever have one section
 */
#define DEFINE_PER_CPU_SECTION(type, name, sec)				\
	__attribute__((section(".data..percpuarrays"))) __typeof__(type) *__pcpu_##name[NR_CPUS]; \
	extern __PCPU_ATTRS("") PER_CPU_DEF_ATTRIBUTES __typeof__(type) name; \
	struct percpu_info __attribute__((section(".data..percpuinfo"))) \
		__per_cpu_info_##name = { (void**)&__pcpu_##name, &name }; \
	__PCPU_ATTRS("") PER_CPU_DEF_ATTRIBUTES __typeof__(type) name

#endif  /* __LINSCHED_PERCPU_H */
#endif
