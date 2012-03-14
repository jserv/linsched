/*
 *  linux/init/version.c
 *
 *  Copyright (C) 1992  Theodore Ts'o
 *
 *  May be freely distributed as part of Linux.
 */

#include <linux/module.h>
#include <linux/uts.h>
#include <linux/utsname.h>


struct uts_namespace init_uts_ns = {
	.kref = {
		.refcount	= ATOMIC_INIT(2),
	},
	.name = {
		.sysname	= "Linux",
		.nodename	= "LinSched",
		.release	= "",
		.version	= "",
		.machine	= "x86_64",
		.domainname	= "(none)",
	},
	.user_ns = &init_user_ns,
};

