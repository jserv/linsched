#include <linux/init.h>

#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/kernel_stat.h>
#include <linux/mc146818rtc.h>
#include <linux/cache.h>
#include <linux/interrupt.h>
#include <linux/cpu.h>
#include <linux/gfp.h>
#include <linux/module.h>

#include <asm/tlbflush.h>
#include <asm/mmu_context.h>
#include <asm/smp.h>

void native_send_call_func_single_ipi(int cpu)
{
}

void native_send_call_func_ipi(const struct cpumask *mask)
{
}

/*
 * this function calls the 'stop' function on all other CPUs in the system.
 */

asmlinkage void smp_reboot_interrupt(void)
{
}

static void native_smp_send_stop(void)
{
}

/*
 * Reschedule call back. Nothing to do,
 * all the work is done automatically when
 * we return from the interrupt.
 */
void smp_reschedule_interrupt(struct pt_regs *regs)
{
}

void smp_call_function_interrupt(struct pt_regs *regs)
{
}

void smp_call_function_single_interrupt(struct pt_regs *regs)
{
}

void native_smp_prepare_boot_cpu(void)
{
}

void native_smp_prepare_cpus(unsigned int max_cpus)
{
}

void native_smp_cpus_done(unsigned int max_cpus)
{
}

int native_cpu_up(unsigned int cpunum)
{
	int oldcpu;
	if (cpu_online(cpunum))
		return -ENOSYS;
	if (!cpu_possible(cpunum))
		return -ENOSYS;

	oldcpu = smp_processor_id();
	linsched_change_cpu(cpunum);

	notify_cpu_starting(cpunum);
	set_cpu_online(cpunum, true);

	linsched_change_cpu(oldcpu);

	return 0;
}

int native_cpu_disable(void)
{
	set_cpu_online(smp_processor_id(), false);
	return 0;
}

void native_cpu_die(unsigned int cpu)
{
}

void native_play_dead(void)
{
}

int on_each_cpu(smp_call_func_t func, void *info, int wait)
{
	return 0;
}

extern void linsched_trigger_cpu(int cpu);


int smp_call_function_single(int cpu, void (*func) (void *info), void *info, int wait)
{
	int curr_cpu = smp_processor_id();

	if (curr_cpu != cpu)
		linsched_change_cpu(cpu);

	/*
	 * We ignore wait since we can only run single-threaded.
	 * but it would need to be
	 * fixed at some point in time by advancing the clock and all.
	 */

	func(info);

	if (curr_cpu != cpu)
		linsched_change_cpu(curr_cpu);

	return 0;
}

struct smp_ops smp_ops = {
	.smp_prepare_boot_cpu	= native_smp_prepare_boot_cpu,
	.smp_prepare_cpus	= native_smp_prepare_cpus,
	.smp_cpus_done		= native_smp_cpus_done,

	.smp_send_stop		= native_smp_send_stop,
	.smp_send_reschedule	= linsched_trigger_cpu,

	.cpu_up			= native_cpu_up,
	.cpu_die		= native_cpu_die,
	.cpu_disable		= native_cpu_disable,
	.play_dead		= native_play_dead,

	.send_call_func_ipi	= native_send_call_func_ipi,
	.send_call_func_single_ipi = native_send_call_func_single_ipi,
};
EXPORT_SYMBOL_GPL(smp_ops);
