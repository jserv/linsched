#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>

unsigned long __per_cpu_offset[NR_CPUS];

extern struct percpu_info __per_cpu_start_info, __per_cpu_end_info;

extern void __per_cpu_start, __per_cpu_end;

static size_t per_cpu_allocation;


void *__alloc_percpu(size_t size, size_t align)
{
	BUG_ON(size > per_cpu_allocation);
	return kmalloc(nr_cpu_ids * per_cpu_allocation, GFP_KERNEL) -
		__per_cpu_offset[0];
}
EXPORT_SYMBOL_GPL(__alloc_percpu);

void free_percpu(void *pdata)
{
	kfree(pdata + __per_cpu_offset[0]);
}

void setup_per_cpu_areas(void)
{
	size_t size = &__per_cpu_end - &__per_cpu_start;
	size_t alloc = round_up(size, PAGE_SIZE);
	void *per_cpu_data = kmalloc(nr_cpu_ids * alloc, GFP_KERNEL);
	int i;
	struct percpu_info *info;

	per_cpu_allocation = alloc;
	for (i = 0; i < nr_cpu_ids; i++) {
		memcpy(per_cpu_data + alloc * i, &__per_cpu_start, size);
		__per_cpu_offset[i] = (unsigned long)per_cpu_data + alloc * i -
			(unsigned long)&__per_cpu_start;
	}

	for(info = &__per_cpu_start_info; info < &__per_cpu_end_info; info++) {
		for (i = 0; i < nr_cpu_ids; i++) {
			info->array[i] = info->base + __per_cpu_offset[i];
		}
	}
}
