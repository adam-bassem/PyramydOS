#include "apic.hpp"
#include <arch/x86_64/acpi/acpi.hpp>
#include <arch/x86_64/cpuid.hpp>
#include <console/console.hpp>
#include <utils.hpp>

struct
{
	
} *madt;

void arch::x86_64::apic::init()
{
	cpuid_result r = arch::x86_64::cpuid::cpuid(1, 0);
	if (!(r.edx & (1ULL << 9)))
	{
		console::kprintf(ANSI_BOLD ANSI_RED "CPU does not support APIC!");
		console::kprintf("Halting...");
		hcf();
	}
	console::kprintf("APIC supported");

	
}
