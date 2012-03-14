#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/module.h>

struct cache_sizes malloc_sizes[] = {
#define CACHE(x) { .cs_size = (x) },
#include <linux/kmalloc_sizes.h>
	CACHE(ULONG_MAX)
#undef CACHE
};

struct mm_struct init_mm = {
	.mm_rb          = RB_ROOT,
	.mm_users       = ATOMIC_INIT(2),
	.mm_count       = ATOMIC_INIT(1),
	.mmap_sem       = __RWSEM_INITIALIZER(init_mm.mmap_sem),
	.page_table_lock =  __SPIN_LOCK_UNLOCKED(init_mm.page_table_lock),
	.mmlist         = LIST_HEAD_INIT(init_mm.mmlist),
};

void kmem_cache_free(struct kmem_cache *cachep, void *objp)
{
	kfree(objp);
}

void *kmem_cache_alloc(struct kmem_cache *cachep, gfp_t flags)
{
	return kzalloc(cachep->buffer_size, flags);
}

void __init kmalloc_init(void)
{
	int i;

	for (i=0;i<sizeof(malloc_sizes)/sizeof(malloc_sizes[0]);i++) {
		struct kmem_cache *cache = malloc(sizeof(struct kmem_cache));

		cache->buffer_size = malloc_sizes[i].cs_size;
		malloc_sizes[i].cs_cachep = cache;
	}
}

void *kmem_cache_alloc_node(struct kmem_cache *cachep, gfp_t flags, int node)
{
	return kmalloc(cachep->buffer_size, flags);
}

void *__kmalloc(size_t size, gfp_t flags)
{
	void *res = malloc(size);
	if (res && (flags & __GFP_ZERO)) {
		memset(res, 0, size);
	}
	return res;
}

void *__kmalloc_node(size_t size, gfp_t flags, int node)
{
	return __kmalloc(size, flags);
}

void kfree(const void *block)
{
	free((void *)block);
}

void fput(struct file *f)
{
}

void destroy_context(struct mm_struct *mm)
{
}

void exit_mmap(struct mm_struct *mm)
{
}

struct mm_struct *swap_token_mm;

void __put_swap_token(struct mm_struct *mm)
{
}

int init_new_context(struct task_struct *task, struct mm_struct *mm)
{
	return 0;
}

struct kmem_cache *
kmem_cache_create(const char *name, size_t size, size_t align,
		unsigned long flags, void (*ctor)(void *))
{
	struct kmem_cache *cache = kzalloc(sizeof(struct kmem_cache),
			GFP_KERNEL);

	cache->buffer_size = size;

	return cache;
}

void __init mmap_init(void)
{
}

struct pglist_data __refdata contig_page_data;
EXPORT_SYMBOL(contig_page_data);

/*
 * TODO: This has to fixed up for account_kernel_stack function
 */
unsigned long __phys_addr(unsigned long x)
{
	/*
	 * We have a 1:1 mapping since we don't care about it.
	 */
	return x;
}

struct mempolicy *__mpol_dup(struct mempolicy *old)
{
	return NULL;
}

void mpol_fix_fork_child_flag(struct task_struct *p)
{
}

void __mpol_put(struct mempolicy *pol)
{
}

/*
 * we don't support capabilities, or memory management. Assume that it
 * succeeds
 */
int cap_vm_enough_memory(struct mm_struct *mm, long pages)
{
	return 0;
}

int anon_vma_fork(struct vm_area_struct *vma, struct vm_area_struct *pvma)
{
	return 0;
}

void vma_prio_tree_add(struct vm_area_struct *vma, struct vm_area_struct *old)
{
}

void __vma_link_rb(struct mm_struct *mm, struct vm_area_struct *vma,
		struct rb_node **rb_link, struct rb_node *rb_parent)
{
}

int copy_page_range(struct mm_struct *dst_mm, struct mm_struct *src_mm,
		struct vm_area_struct *vma)
{
	return 0;
}

void flush_tlb_mm(struct mm_struct *mm)
{
}

int pgd_alloc(struct mm_struct *mm) {
	return 0;
}

void pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
	BUG();
}

void exit_aio(struct mm_struct *mm)
{
}
