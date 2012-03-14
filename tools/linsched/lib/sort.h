#ifndef LINSCHED_LIB_SORT
#define LINSCHED_LIB_SORT

#include <string.h>

/* Quicksort implementation from glibc, adapted into a
 * type-specialized version optimized for reasonably small structs.
 * This cuts about 1/3 of the time off of load balance scoring over qsort(3).
 */

/* The next 4 #defines implement a very fast in-line stack abstraction. */
/* The stack needs log (total_elements) entries (we could even subtract
   log(MAX_THRESH)).  Since total_elements has type size_t, we get as
   upper bound for log (total_elements):
   bits per byte (CHAR_BIT) * sizeof(size_t).  */
#define QSORT_STACK_SIZE	(8 * sizeof(size_t))
#define QSORT_PUSH(low, high)	((void) ((top->lo = (low)), (top->hi = (high)), ++top))
#define	QSORT_POP(low, high)	((void) (--top, (low = top->lo), (high = top->hi)))
#define	QSORT_STACK_NOT_EMPTY	(stack < top)


/* Order size using quicksort.  This implementation incorporates
   four optimizations discussed in Sedgewick:

   1. Non-recursive, using an explicit stack of pointer that store the
   next array partition to sort.  To save time, this maximum amount
   of space required to store an array of SIZE_MAX is allocated on the
   stack.  Assuming a 32-bit (64 bit) integer for size_t, this needs
   only 32 * sizeof(stack_node) == 256 bytes (for 64 bit: 1024 bytes).
   Pretty cheap, actually.

   2. Chose the pivot element using a median-of-three decision tree.
   This reduces the probability of selecting a bad pivot value and
   eliminates certain extraneous comparisons.

   3. Only quicksorts TOTAL_ELEMS / MAX_THRESH partitions, leaving
   insertion sort to order the MAX_THRESH items within each partition.
   This is a big win, since insertion sort is faster for small, mostly
   sorted array segments.

   4. The larger of the two sub-partitions is always pushed onto the
   stack first, with the algorithm then concentrating on the
   smaller partition.  This *guarantees* no more than log (total_elems)
   stack size is needed (actually O(1) in this case)!  */


/* Defines a function qsort_<name> that compares using the given function */
#define define_specialized_qsort(name, type, compare)			\
	static inline void _qsort_swap##name(type *a, type *b)		\
	{								\
		type temp = *a;						\
		*a = *b;						\
		*b = temp;						\
	}								\
	void qsort_##name(type *base_ptr, size_t total_elems)		\
	{								\
		static const int MAX_THRESH = 4;			\
		typedef struct {					\
			void *lo;					\
			void *hi;					\
		} stack_node;						\
									\
									\
		if (total_elems == 0)					\
			/* Avoid lossage with unsigned arithmetic below.  */ \
			return;						\
									\
		if (total_elems > MAX_THRESH)				\
		{							\
			type *lo = base_ptr;				\
			type *hi = &lo[total_elems - 1];		\
			stack_node stack[QSORT_STACK_SIZE];		\
			stack_node *top = stack + 1;			\
									\
			while (QSORT_STACK_NOT_EMPTY)			\
			{						\
				type *left_ptr;				\
				type *right_ptr;			\
									\
				/* Select median value from among LO, MID, and HI. Rearrange \
				   LO and HI so the three values are sorted. This lowers the \
				   probability of picking a pathological pivot value and \
				   skips a comparison for both the LEFT_PTR and RIGHT_PTR in \
				   the while loops. */			\
									\
				type *mid = lo + ((hi - lo) >> 1);	\
									\
				if (compare(mid, lo) < 0)		\
					_qsort_swap##name (mid, lo);	\
				if (compare (hi,  mid) < 0)		\
					_qsort_swap##name (mid, hi);	\
				else					\
					goto jump_over;			\
				if (compare ((void *) mid, (void *) lo) < 0) \
					_qsort_swap##name (mid, lo);	\
			jump_over:;					\
									\
				left_ptr  = lo + 1;			\
				right_ptr = hi - 1;			\
									\
				/* Here's the famous ``collapse the walls'' section of quicksort. \
				   Gotta like those tight inner loops!  They are the main reason \
				   that this algorithm runs much faster than others. */	\
				do					\
				{					\
					while (compare (left_ptr, mid) < 0) \
						left_ptr ++;		\
									\
					while (compare (mid, right_ptr) < 0) \
						right_ptr --;		\
									\
					if (left_ptr < right_ptr)	\
					{				\
						_qsort_swap##name (left_ptr, right_ptr); \
						if (mid == left_ptr)	\
							mid = right_ptr; \
						else if (mid == right_ptr) \
							mid = left_ptr;	\
						left_ptr ++;		\
						right_ptr --;		\
					}				\
					else if (left_ptr == right_ptr)	\
					{				\
						left_ptr ++;		\
						right_ptr --;		\
						break;			\
					}				\
				}					\
				while (left_ptr <= right_ptr);		\
									\
				/* Set up pointers for next iteration.  First determine whether	\
				   left and right partitions are below the threshold size.  If so, \
				   ignore one or both.  Otherwise, push the larger partition's \
				   bounds on the stack and continue sorting the smaller one. */	\
									\
				if ((right_ptr - lo) <= MAX_THRESH)	\
				{					\
					if ((hi - left_ptr) <= MAX_THRESH) \
						/* Ignore both small partitions. */ \
						QSORT_POP (lo, hi);	\
					else				\
						/* Ignore small left partition. */ \
						lo = left_ptr;		\
				}					\
				else if ((hi - left_ptr) <= MAX_THRESH)	\
					/* Ignore small right partition. */ \
					hi = right_ptr;			\
				else if ((right_ptr - lo) > (hi - left_ptr)) \
				{					\
					/* Push larger left partition indices. */ \
					QSORT_PUSH (lo, right_ptr);	\
					lo = left_ptr;			\
				}					\
				else					\
				{					\
					/* Push larger right partition indices. */ \
					QSORT_PUSH (left_ptr, hi);	\
					hi = right_ptr;			\
				}					\
			}						\
		}							\
									\
		/* Once the BASE_PTR array is partially sorted by quicksort the rest \
		   is completely sorted using insertion sort, since this is efficient \
		   for partitions below MAX_THRESH size. BASE_PTR points to the beginning \
		   of the array to sort, and END_PTR points at the very last element in	\
		   the array (*not* one beyond it!). */			\
									\
		{							\
			type *const end_ptr = &base_ptr[(total_elems - 1)]; \
			type *tmp_ptr = base_ptr;			\
			type *thresh = min(end_ptr, base_ptr + MAX_THRESH); \
			type *run_ptr;					\
									\
			/* Find smallest element in first threshold and place it at the	\
			   array's beginning.  This is the smallest array element, \
			   and the operation speeds up insertion sort's inner loop. */ \
									\
			for (run_ptr = tmp_ptr + 1; run_ptr <= thresh; run_ptr ++) \
				if (compare (run_ptr, tmp_ptr) < 0)	\
					tmp_ptr = run_ptr;		\
									\
			if (tmp_ptr != base_ptr)			\
				_qsort_swap##name (tmp_ptr, base_ptr);	\
									\
			/* Insertion sort, running from left-hand-side up to right-hand-side.  */ \
									\
			run_ptr = base_ptr + 1;				\
			while ((run_ptr += 1) <= end_ptr)		\
			{						\
				tmp_ptr = run_ptr - 1;			\
				while (compare(run_ptr, tmp_ptr) < 0)	\
					tmp_ptr --;			\
									\
				tmp_ptr ++;				\
				if (tmp_ptr != run_ptr)			\
				{					\
					type temp = *run_ptr;		\
									\
					memmove(tmp_ptr + 1, tmp_ptr, sizeof(type) * (run_ptr - tmp_ptr)); \
					*tmp_ptr = temp;		\
				}					\
			}						\
		}							\
	}


#endif
