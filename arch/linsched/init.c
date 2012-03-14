/*
 * arch/linsched/init.c
 *
 * Bootstrap a LinSched machine.
 */

#include <linux/types.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/cgroup.h>
#include <linux/tick.h>

static void __init boot_cpu_init(void)
{
	int cpu = smp_processor_id();
	/* Mark the boot cpu "present", "online" etc for SMP and UP case */
	set_cpu_online(cpu, true);
	set_cpu_active(cpu, true);
	set_cpu_present(cpu, true);
	set_cpu_possible(cpu, true);
}

static noinline int init_post(void);
static noinline void __init_refok rest_init(void)
	__releases(kernel_lock)
{
	/* kernel_init thread: just execute kernel_init. */
	init_post();

	/* initialize, and create an idle task for, every cpu. */
	int cpu_id;
	struct task_struct *idle;
	for (cpu_id = 0; cpu_id < nr_cpu_ids; cpu_id++) {
		idle = fork_idle(cpu_id);
		per_cpu(current_task, cpu_id) = idle;
	}

	/* Don't need kthreadd thread. */
	/*
	 * The boot idle thread must execute schedule()
	 * at least once to get things moving:
	 */
	init_idle_bootup_task(current);
}

void kmalloc_init(void);

asmlinkage void __init start_kernel(void)
{
	kmalloc_init();

	cgroup_init_early();

	tick_init();
	boot_cpu_init();
	setup_per_cpu_areas();

	/*
	 * Set up the scheduler prior starting any interrupts (such as the
	 * timer interrupt). Full topology setup happens at smp_init()
	 * time - but meanwhile we still have a functioning scheduler.
	 */
	sched_init();
	init_timers();
	hrtimers_init();
	timekeeping_init();
	sched_clock_init();
	proc_caches_init();
	/* Do the rest non-__init'ed, we're now alive */
	rest_init();
}

static noinline int init_post(void)
	__releases(kernel_lock)
{
	/* fake do_pre_smp_initcalls(); */
	extern initcall_t __initcall_migration_initearly;
	__initcall_migration_initearly();
	smp_init();
	sched_init_smp();
	return 0;
}
//		${LINUXDIR}/init/main.o 
