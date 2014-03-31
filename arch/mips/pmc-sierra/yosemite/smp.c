#include <linux/linkage.h>
#include <linux/sched.h>
#include <linux/smp.h>

#include <asm/pmon.h>
#include <asm/titan_dep.h>
#include <asm/time.h>

#define LAUNCHSTACK_SIZE 256

static __cpuinitdata arch_spinlock_t launch_lock = __ARCH_SPIN_LOCK_UNLOCKED;

static unsigned long secondary_sp __cpuinitdata;
static unsigned long secondary_gp __cpuinitdata;

static unsigned char launchstack[LAUNCHSTACK_SIZE] __initdata
	__attribute__((aligned(2 * sizeof(long))));

static void __init prom_smp_bootstrap(void)
{
	local_irq_disable();

	while (arch_spin_is_locked(&launch_lock));

	__asm__ __volatile__(
	"	move	$sp, %0		\n"
	"	move	$gp, %1		\n"
	"	j	smp_bootstrap	\n"
	:
	: "r" (secondary_sp), "r" (secondary_gp));
}

void __init prom_grab_secondary(void)
{
	arch_spin_lock(&launch_lock);

	pmon_cpustart(1, &prom_smp_bootstrap,
	              launchstack + LAUNCHSTACK_SIZE, 0);
}

void titan_mailbox_irq(void)
{
	int cpu = smp_processor_id();
	unsigned long status;

	switch (cpu) {
	case 0:
		status = OCD_READ(RM9000x2_OCD_INTP0STATUS3);
		OCD_WRITE(RM9000x2_OCD_INTP0CLEAR3, status);

		if (status & 0x2)
			smp_call_function_interrupt();
		if (status & 0x4)
			scheduler_ipi();
		break;

	case 1:
		status = OCD_READ(RM9000x2_OCD_INTP1STATUS3);
		OCD_WRITE(RM9000x2_OCD_INTP1CLEAR3, status);

		if (status & 0x2)
			smp_call_function_interrupt();
		if (status & 0x4)
			scheduler_ipi();
		break;
	}
}

static void yos_send_ipi_single(int cpu, unsigned int action)
{
	switch (action) {
	case SMP_RESCHEDULE_YOURSELF:
		if (cpu == 1)
			OCD_WRITE(RM9000x2_OCD_INTP1SET3, 4);
		else
			OCD_WRITE(RM9000x2_OCD_INTP0SET3, 4);
		break;

	case SMP_CALL_FUNCTION:
		if (cpu == 1)
			OCD_WRITE(RM9000x2_OCD_INTP1SET3, 2);
		else
			OCD_WRITE(RM9000x2_OCD_INTP0SET3, 2);
		break;
	}
}

static void yos_send_ipi_mask(const struct cpumask *mask, unsigned int action)
{
	unsigned int i;

	for_each_cpu(i, mask)
		yos_send_ipi_single(i, action);
}

static void __cpuinit yos_init_secondary(void)
{
	set_c0_status(ST0_CO | ST0_IE | ST0_IM);
}

static void __cpuinit yos_smp_finish(void)
{
}

static void yos_cpus_done(void)
{
}

static void __cpuinit yos_boot_secondary(int cpu, struct task_struct *idle)
{
	unsigned long gp = (unsigned long) task_thread_info(idle);
	unsigned long sp = __KSTK_TOS(idle);

	secondary_sp = sp;
	secondary_gp = gp;

	arch_spin_unlock(&launch_lock);
}

static void __init yos_smp_setup(void)
{
	int i;

	init_cpu_possible(cpu_none_mask);

	for (i = 0; i < 2; i++) {
		set_cpu_possible(i, true);
		__cpu_number_map[i]	= i;
		__cpu_logical_map[i]	= i;
	}
}

static void __init yos_prepare_cpus(unsigned int max_cpus)
{
	if (num_possible_cpus())
		set_c0_status(STATUSF_IP5);
}

struct plat_smp_ops yos_smp_ops = {
	.send_ipi_single	= yos_send_ipi_single,
	.send_ipi_mask		= yos_send_ipi_mask,
	.init_secondary		= yos_init_secondary,
	.smp_finish		= yos_smp_finish,
	.cpus_done		= yos_cpus_done,
	.boot_secondary		= yos_boot_secondary,
	.smp_setup		= yos_smp_setup,
	.prepare_cpus		= yos_prepare_cpus,
};
