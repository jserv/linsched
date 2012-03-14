#ifndef _ASM_LINSCHED_HWEIGHT_H
#define _ASM_LINSCHED_HWEIGHT_H

static inline unsigned int __arch_hweight32(unsigned int w)
{
	return __sw_hweight32(w);
}

static inline unsigned int __arch_hweight16(unsigned int w)
{
	return __arch_hweight32(w & 0xffff);
}

static inline unsigned int __arch_hweight8(unsigned int w)
{
	return __arch_hweight32(w & 0xff);
}

static inline unsigned long __arch_hweight64(__u64 w)
{
	return __sw_hweight64(w);
}

#endif
