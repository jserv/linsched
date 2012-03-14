#ifndef _ASM_LINSCHED_UACCESS_H
#define _ASM_LINSCHED_UACCESS_H

#define VERIFY_READ	0
#define VERIFY_WRITE	1

#define __get_user(...) 0
#define __put_user(x, ptr) sizeof(*ptr)
#define put_user(x, ptr) sizeof(*ptr)
#define get_user(...) 0
#define copy_to_user(...) 0
#define copy_from_user(...) 0
#define __copy_from_user(...) 0
#define __copy_to_user(...) 0
#define __copy_from_user_inatomic(...) 0
#define access_ok(...) 1

#define MAKE_MM_SEG(s)	((mm_segment_t) { (s) })
#define KERNEL_DS	MAKE_MM_SEG(~0UL)

struct exception_table_entry {
	unsigned long insn, fixup;
};

#endif /* _ASM_LINSCHED_UACCESS_H */
