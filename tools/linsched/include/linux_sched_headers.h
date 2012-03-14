#ifndef __LINSCHED_LINUX_SCHED_HEADERS_H
#define __LINSCHED_LINUX_SCHED_HEADERS_H

#ifdef _LINUX_SCHED_H
#error linux_sched_headers.h must be included before sched.h
#endif

#define __sigset_t_defined
#define __defined_schedparam
#define __dev_t_defined
#define __timer_t_defined
#define __loff_t_defined
#define __blkcnt_t_defined
#define __int8_t_defined
#define __need_schedparam
#define _SYS_TYPES_H
#define _SCHED_H
#define _TIME_H
#include "../kernel/sched/sched.h"
#include <linux/cgroup.h>
#include <linux/sched.h>
#include <linux/stop_machine.h>
#include "nohz.h"
#undef abs

#endif /* __LINSCHED_LINUX_SCHED_HEADERS_H */
