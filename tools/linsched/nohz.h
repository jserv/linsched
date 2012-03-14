#ifndef __LINSCHED_HEADER_NOHZ
#define __LINSCHED_HEADER_NOHZ

struct nohz {
	cpumask_var_t idle_cpus_mask;
	atomic_t nr_cpus;
	unsigned long next_balance;
};

#endif
