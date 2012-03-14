CC = ${CROSS_COMPILE}gcc

CFLAGS = -g -O2 -m64 -D__KERNEL__ -D__LINSCHED__ -Wall -Wundef -Wstrict-prototypes \
	 -Werror-implicit-function-declaration -fno-common \
	 -I${LINSCHED_DIR}/include  -I${LINUXDIR}/include \
	 -I${LINUXDIR}/arch/linsched/include -I${LINSCHED_DIR}/ \
         -include ${LINSCHED_DIR}/include/generated/autoconf.h \
	 -Wno-pointer-sign -include ${LINUXDIR}/include/linux/kconfig.h

# Don't use system headers (such as /usr/include/asm) for the kernel
CFLAGS_LINUX = $(CFLAGS) -nostdinc -isystem $(shell $(CC) -print-file-name=include) \
	       -include ${LINSCHED_DIR}/linux_linsched.h \
	       -Wno-unused  -Wno-strict-aliasing

LFLAGS = -lm

LINSCHED_OBJS = ${LINSCHED_DIR}/linux_linsched.o \
		${LINSCHED_DIR}/numa.o \
		${LINSCHED_DIR}/hrtimer.o \
		${LINSCHED_DIR}/stubs.o \
		${LINSCHED_DIR}/test_lib.o \
		${LINSCHED_DIR}/linsched.o \
		${LINSCHED_DIR}/load_balance_score.o \
		${LINSCHED_DIR}/sanity_check.o \
		${LINSCHED_DIR}/nohz_tracking.o \
		${LINSCHED_DIR}/linsched_rand.o \
		${LINSCHED_DIR}/linsched_sim.o \
		${LINSCHED_DIR}/stubs/sched.o

LINUX_OBJS =	${LINUXDIR}/kernel/notifier.o \
		${LINUXDIR}/kernel/timer.o \
		${LINUXDIR}/kernel/time/clockevents.o \
		${LINUXDIR}/kernel/sched/clock.o \
		${LINUXDIR}/kernel/sched/core.o \
		${LINUXDIR}/kernel/sched/cpupri.o \
		${LINUXDIR}/kernel/sched/debug.o \
		${LINUXDIR}/kernel/sched/idle_task.o \
		${LINUXDIR}/kernel/sched/fair.o \
		${LINUXDIR}/kernel/sched/rt.o \
		${LINUXDIR}/kernel/sched/stats.o \
		${LINUXDIR}/kernel/sched/stop_task.o \
		${LINUXDIR}/kernel/fork.o \
		${LINUXDIR}/arch/linsched/kernel/init_task.o \
		${LINUXDIR}/kernel/exit.o ${LINUXDIR}/kernel/time.o \
		${LINUXDIR}/kernel/cpu.o \
		${LINUXDIR}/kernel/rwsem.o \
		${LINUXDIR}/kernel/mutex.o \
		${LINUXDIR}/kernel/hrtimer.o \
		${LINUXDIR}/kernel/pid.o \
		${LINUXDIR}/kernel/posix-cpu-timers.o \
		${LINUXDIR}/kernel/spinlock.o \
		${LINUXDIR}/kernel/time/timekeeping.o \
		${LINUXDIR}/kernel/time/ntp.o \
		${LINUXDIR}/kernel/time/jiffies.o \
		${LINUXDIR}/kernel/time/tick-common.o \
		${LINUXDIR}/kernel/time/tick-oneshot.o \
		${LINUXDIR}/kernel/time/tick-sched.o \
		${LINUXDIR}/kernel/time/tick-broadcast.o \
		${LINUXDIR}/kernel/wait.o \
		${LINUXDIR}/arch/linsched/init.o \
		${LINUXDIR}/arch/linsched/kernel/process.o \
		${LINUXDIR}/arch/linsched/kernel/smp.o \
		${LINUXDIR}/arch/linsched/kernel/pid.o \
		${LINUXDIR}/arch/linsched/kernel/mm.o \
		${LINUXDIR}/arch/linsched/kernel/io.o \
		${LINUXDIR}/arch/linsched/kernel/fs.o \
		${LINUXDIR}/arch/linsched/kernel/rcu.o \
		${LINUXDIR}/arch/linsched/kernel/irq.o \
		${LINUXDIR}/arch/linsched/kernel/version.o \
		${LINUXDIR}/arch/linsched/kernel/percpu.o \
		${LINUXDIR}/arch/linsched/kernel/lock.o \
		${LINUXDIR}/arch/linsched/kernel/cred.o \
		${LINUXDIR}/arch/linsched/kernel/cgroup.o \
		${LINUXDIR}/arch/linsched/kernel/misc.o \
		${LINUXDIR}/lib/rbtree.o ${LINUXDIR}/lib/div64.o \
		${LINUXDIR}/lib/cpumask.o ${LINUXDIR}/lib/bitmap.o \
		${LINUXDIR}/lib/list_debug.o \
		${LINUXDIR}/lib/find_next_bit.o \
		${LINUXDIR}/lib/hweight.o ${LINUXDIR}/lib/reciprocal_div.o \
		${LINUXDIR}/lib/plist.o \
		${LINUXDIR}/lib/ctype.o ${LINUXDIR}/lib/string.o \
		${LINUXDIR}/lib/hexdump.o ${LINUXDIR}/lib/percpu_counter.o \
		${LINUXDIR}/lib/timerqueue.o \
		${LINUXDIR}/lib/find_last_bit.o \
		${LINUXDIR}/lib/flex_array.o \
		${LINUXDIR}/lib/rwsem-spinlock.o \
		${LINUXDIR}/lib/kstrtox.o

LD_PERCPU = ${LD} -r -T ${LINSCHED_DIR}/linsched.lds

OBJ_FILES = ${LINSCHED_OBJS} ${LINUX_OBJS}
DEPS := ${OBJ_FILES:.o=.d}
-include ${DEPS}

TIME_HDR=${LINUXDIR}/kernel/timeconst.h
${TIME_HDR}: ${LINUXDIR}/kernel/timeconst.pl
	@echo "(Generating timeconst.h)"
	@perl ${LINUXDIR}/kernel/timeconst.pl 1000 > $@

${LINUX_OBJS}: %.o: %.c ${TIME_HDR}
	@echo "CC KERNEL $<"
	@${CC} -o $@ ${CFLAGS_LINUX} -c $< -MMD

%.o: %.c
	@echo "CC SIM $<"
	@${CC} ${CFLAGS} -o $@ -c $< -MMD



