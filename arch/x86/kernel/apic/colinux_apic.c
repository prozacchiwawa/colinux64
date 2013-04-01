#include <linux/init.h>
#include <linux/ftrace.h>
#include <linux/hardirq.h>
#include <linux/clockchips.h>
#include <asm/apic.h>
#include <asm/idle.h>

unsigned int num_processors;

struct apic *apic;

static struct apic colinux_apic = {
};

unsigned long mp_lapic_addr;
int smp_found_config;
unsigned int disabled_cpus;
unsigned int boot_cpu_physical_apicid;
unsigned int max_physical_apicid;

DEFINE_EARLY_PER_CPU_READ_MOSTLY(u16, x86_cpu_to_apicid, BAD_APICID);
DEFINE_EARLY_PER_CPU_READ_MOSTLY(u16, x86_bios_cpu_apicid, BAD_APICID);
EXPORT_EARLY_PER_CPU_SYMBOL(x86_cpu_to_apicid);
EXPORT_EARLY_PER_CPU_SYMBOL(x86_bios_cpu_apicid);

int first_system_vector = 0xfe;

unsigned int apic_verbosity;
int apic_version[MAX_LOCAL_APIC];
physid_mask_t phys_cpu_present_map;

static void lapic_timer_setup(enum clock_event_mode mode,
			      struct clock_event_device *evt)
{
}

static int lapic_next_event(unsigned long delta,
			    struct clock_event_device *evt)
{
	return 0;
}

static void lapic_timer_broadcast(const struct cpumask *mask)
{
#ifdef CONFIG_SMP
	apic->send_IPI_mask(mask, LOCAL_TIMER_VECTOR);
#endif
}

/*
 * The local apic timer can be used for any function which is CPU local.
 */
static struct clock_event_device lapic_clockevent = {
	.name		= "lapic",
	.features	= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT
			| CLOCK_EVT_FEAT_C3STOP | CLOCK_EVT_FEAT_DUMMY,
	.shift		= 32,
	.set_mode	= lapic_timer_setup,
	.set_next_event	= lapic_next_event,
	.broadcast	= lapic_timer_broadcast,
	.rating		= 100,
	.irq		= -1,
};
static DEFINE_PER_CPU(struct clock_event_device, lapic_events);

void __init setup_boot_APIC_clock(void)
{
	struct clock_event_device *levt = &__get_cpu_var(lapic_events);
	memcpy(levt, &lapic_clockevent, sizeof(*levt));
	levt->cpumask = cpumask_of(smp_processor_id());
	clockevents_register_device(levt);
}

int __init APIC_init_uniprocessor(void) 
{
	apic = &colinux_apic;
	return 0;
}

void disable_local_APIC(void) 
{
}

void lapic_shutdown(void) 
{
}

void init_apic_mappings(void) 
{
}

int setup_APIC_eilvt(u8 offset, u8 vector, u8 msg_type, u8 mask) 
{
	return 0;
}

void __cpuinit generic_processor_info(int apicid, int version)
{
	int cpu, max = nr_cpu_ids;
	bool boot_cpu_detected = physid_isset(boot_cpu_physical_apicid,
				phys_cpu_present_map);

	/*
	 * If boot cpu has not been detected yet, then only allow upto
	 * nr_cpu_ids - 1 processors and keep one slot free for boot cpu
	 */
	if (!boot_cpu_detected && num_processors >= nr_cpu_ids - 1 &&
	    apicid != boot_cpu_physical_apicid) {
		int thiscpu = max + disabled_cpus - 1;

		pr_warning(
			"ACPI: NR_CPUS/possible_cpus limit of %i almost"
			" reached. Keeping one slot for boot cpu."
			"  Processor %d/0x%x ignored.\n", max, thiscpu, apicid);

		disabled_cpus++;
		return;
	}

	if (num_processors >= nr_cpu_ids) {
		int thiscpu = max + disabled_cpus;

		pr_warning(
			"ACPI: NR_CPUS/possible_cpus limit of %i reached."
			"  Processor %d/0x%x ignored.\n", max, thiscpu, apicid);

		disabled_cpus++;
		return;
	}

	num_processors++;
	if (apicid == boot_cpu_physical_apicid) {
		/*
		 * x86_bios_cpu_apicid is required to have processors listed
		 * in same order as logical cpu numbers. Hence the first
		 * entry is BSP, and so on.
		 * boot_cpu_init() already hold bit 0 in cpu_present_mask
		 * for BSP.
		 */
		cpu = 0;
	} else
		cpu = cpumask_next_zero(-1, cpu_present_mask);

	/*
	 * Validate version
	 */
	if (version == 0x0) {
		pr_warning("BIOS bug: APIC version is 0 for CPU %d/0x%x, fixing up to 0x10\n",
			   cpu, apicid);
		version = 0x10;
	}
	apic_version[apicid] = version;

	if (version != apic_version[boot_cpu_physical_apicid]) {
		pr_warning("BIOS bug: APIC version mismatch, boot CPU: %x, CPU %d: version %x\n",
			apic_version[boot_cpu_physical_apicid], cpu, version);
	}

	physid_set(apicid, phys_cpu_present_map);
	if (apicid > max_physical_apicid)
		max_physical_apicid = apicid;

#if defined(CONFIG_SMP) || defined(CONFIG_X86_64)
	early_per_cpu(x86_cpu_to_apicid, cpu) = apicid;
	early_per_cpu(x86_bios_cpu_apicid, cpu) = apicid;
#endif
#ifdef CONFIG_X86_32
	early_per_cpu(x86_cpu_to_logical_apicid, cpu) =
		apic->x86_32_early_logical_apicid(cpu);
#endif
	set_cpu_possible(cpu, true);
	set_cpu_present(cpu, true);
}

void __init register_lapic_address(unsigned long address) 
{
	mp_lapic_addr = address;
}

/*
 * The guts of the apic timer interrupt
 */
static void local_apic_timer_interrupt(void)
{
	int cpu = smp_processor_id();
	struct clock_event_device *evt = &per_cpu(lapic_events, cpu);

	/*
	 * Normally we should not be here till LAPIC has been initialized but
	 * in some cases like kdump, its possible that there is a pending LAPIC
	 * timer interrupt from previous kernel's context and is delivered in
	 * new kernel the moment interrupts are enabled.
	 *
	 * Interrupts are enabled early and LAPIC is setup much later, hence
	 * its possible that when we get here evt->event_handler is NULL.
	 * Check for event_handler being NULL and discard the interrupt as
	 * spurious.
	 */
	if (!evt->event_handler) {
		pr_warning("Spurious LAPIC timer interrupt on cpu %d\n", cpu);
		/* Switch it off */
		lapic_timer_setup(CLOCK_EVT_MODE_SHUTDOWN, evt);
		return;
	}

	/*
	 * the NMI deadlock-detector uses this.
	 */
	inc_irq_stat(apic_timer_irqs);

	evt->event_handler(evt);
}

void __irq_entry smp_apic_timer_interrupt(struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);

	/*
	 * NOTE! We'd better ACK the irq immediately,
	 * because timer handling can be slow.
	 */
	ack_APIC_irq();
	/*
	 * update_process_times() expects us to have done irq_enter().
	 * Besides, if we don't timer interrupts ignore the global
	 * interrupt lock, which is the WrongThing (tm) to do.
	 */
	irq_enter();
	exit_idle();
	local_apic_timer_interrupt();
	irq_exit();

	set_irq_regs(old_regs);
}

void smp_spurious_interrupt(struct pt_regs *regs)
{
	u32 v;

	irq_enter();
	exit_idle();
	/*
	 * Check if this really is a spurious interrupt and ACK it
	 * if it is a vectored one.  Just in case...
	 * Spurious interrupts should not be ACKed.
	 */
#if 0
	v = apic_read(APIC_ISR + ((SPURIOUS_APIC_VECTOR & ~0x1f) >> 1));
	if (v & (1 << (SPURIOUS_APIC_VECTOR & 0x1f)))
		ack_APIC_irq();
#endif

	inc_irq_stat(irq_spurious_count);

	/* see sw-dev-man vol 3, chapter 7.4.13.5 */
	pr_info("spurious APIC interrupt on CPU#%d, "
		"should never happen.\n", smp_processor_id());
	irq_exit();
}

void smp_error_interrupt(struct pt_regs *regs)
{
	u32 v0, v1;
	u32 i = 0;
	static const char * const error_interrupt_reason[] = {
		"Send CS error",		/* APIC Error Bit 0 */
		"Receive CS error",		/* APIC Error Bit 1 */
		"Send accept error",		/* APIC Error Bit 2 */
		"Receive accept error",		/* APIC Error Bit 3 */
		"Redirectable IPI",		/* APIC Error Bit 4 */
		"Send illegal vector",		/* APIC Error Bit 5 */
		"Received illegal vector",	/* APIC Error Bit 6 */
		"Illegal register address",	/* APIC Error Bit 7 */
	};

	irq_enter();
	exit_idle();
#if 0
	/* First tickle the hardware, only then report what went on. -- REW */
	v0 = apic_read(APIC_ESR);
	apic_write(APIC_ESR, 0);
	v1 = apic_read(APIC_ESR);
	ack_APIC_irq();
	atomic_inc(&irq_err_count);

	apic_printk(APIC_DEBUG, KERN_DEBUG "APIC error on CPU%d: %02x(%02x)",
		    smp_processor_id(), v0 , v1);

	v1 = v1 & 0xff;
	while (v1) {
		if (v1 & 0x1)
			apic_printk(APIC_DEBUG, KERN_CONT " : %s", error_interrupt_reason[i]);
		i++;
		v1 >>= 1;
	}

	apic_printk(APIC_DEBUG, KERN_CONT "\n");
#endif

	irq_exit();
}

void __cpuinit setup_secondary_APIC_clock(void)
{
}

