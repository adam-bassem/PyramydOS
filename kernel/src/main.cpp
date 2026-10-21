#include <arch/x86_64/gdt/gdt.hpp>
#include <arch/x86_64/idt/idt.hpp>
#include <arch/x86_64/mm/pmm.hpp>
#include <arch/x86_64/mm/vmm.hpp>
#include	<arch/x86_64/acpi/acpi.hpp>
#include <arch/x86_64/io.hpp>
#include <arch/x86_64/cpuid.hpp>
#include <arch/x86_64/apic/apic.hpp>
#include <arch/x86_64/apic/ioapic.hpp>
#include <arch/x86_64/apic/lapic.hpp>
#include <arch/x86_64/irq/irq.hpp>

#include "utils.hpp"
#include <console/console.hpp>
#include <allocator/allocator.hpp>
#include <timers/timers.hpp>
#include <interrupts/ints.hpp>
#include <pcie/pcie.hpp>

// Kernel starts here
extern "C" void kmain() {
    pre_kernel(); // some weird stuff i literally forgot what they were

	void* heap_base;

	arch::x86_64::gdt::init();
	arch::x86_64::idt::init();
	arch::x86_64::pmm::init();
	arch::x86_64::vmm::init();

	// allocate 64KiB for stack and put it in rsp0 in the TSS
	void* stack_base = arch::x86_64::vmm::alloc_pages(16);
	arch::x86_64::gdt::set_rsp0(stack_base);
	asm volatile("mov %0, %%rsp" : : "r"(stack_base) : "rsp");

	// just allocate one page the heap grows anyway
	heap_base = arch::x86_64::vmm::alloc_pages(1);
    alloc::init(heap_base, 1);

    // get text printin'
	console::init();

	// some fancy shit
	console::kprintf("****************************************************************************");
	console::kprintf("* ________                                         _______________________ *");
	console::kprintf("* ___  __ \\____  ______________ _______ ________  _______  /_  __ \\_  ___/ *");
	console::kprintf("* __  /_/ /_  / / /_  ___/  __ `/_  __ `__ \\_  / / /  __  /_  / / /____ \\  *");
	console::kprintf("* _  ____/_  /_/ /_  /   / /_/ /_  / / / / /  /_/ // /_/ / / /_/ /____/ /  *");
	console::kprintf("* /_/     _\\__, / /_/    \\__,_/ /_/ /_/ /_/_\\__, / \\__,_/  \\____/ /____/   *");
	console::kprintf("*         /____/                           /____/                          *");
	console::kprintf("****************************************************************************");
	console::kprintf("Copyright \xa9 2026 Adam Bassem. All rights reserved.");

	// some hardware shit cuz why not?
	// RAM info
	uint64_t ram_pages = arch::x86_64::pmm::total_pages_count();
	uint64_t ram_bytes = ram_pages * 0x1000ULL;

	uint64_t ram_gib = ram_bytes / (0x400 * 0x400 * 0x400);
	uint64_t ram_remainder = ram_bytes % (0x400 * 0x400 * 0x400);

	uint64_t ram_decimal = (ram_remainder * 10) / (0x400 * 0x400 * 0x400);

	bool has_decimal = ram_decimal != 0;

	// CPU vendor
	char vendor[13];

	struct cpuid_result r = arch::x86_64::cpuid::cpuid(0, 0);

	*(uint32_t*)&vendor[0] = r.ebx;
	*(uint32_t*)&vendor[4] = r.edx;
	*(uint32_t*)&vendor[8] = r.ecx;

	vendor[12] = '\0';

	// CPU brand
	char brand[49];
	bool has_brand = false;

	r = arch::x86_64::cpuid::cpuid(0x80000000, 0);

	uint32_t max_extended_leaf = r.eax;

	if (max_extended_leaf >= 0x80000004)
	{
		for (uint32_t i = 0; i < 3; i++)
		{
			r = arch::x86_64::cpuid::cpuid(0x80000002 + i, 0);

			*(uint32_t*)&brand[i * 16 + 0]  = r.eax;
			*(uint32_t*)&brand[i * 16 + 4]  = r.ebx;
			*(uint32_t*)&brand[i * 16 + 8]  = r.ecx;
			*(uint32_t*)&brand[i * 16 + 12] = r.edx;
		}

		brand[48] = '\0';
		has_brand = true;
	}

	// System information
	console::kprintf("Architecture: x86_64");

	console::kprintf("Got %d framebuffer(s)", framebuffer_request.response->framebuffer_count);

	console::kprintf(
		"CPU: %s%s%s",
		vendor,
		has_brand ? " - " : "",
		has_brand ? brand : ""
	);

	console::print_timestamp();
	printf("RAM: %llu", ram_gib);

	if (has_decimal)
		printf(".%u", (uint32_t)ram_decimal);

	printf(" GiB\r\n" ANSI_RESET);

    // printed late because console wasn't initialised
	console::kprintf("GDT Initialised...");
	console::kprintf("IDT Initialised...");
	console::kprintf("Physical Memory Manager Initialised...");
	console::kprintf("Virtual Memory Manager Initialised...");

	console::kprintf("Allocator Initialised...");

	// acpi you evil piece of shit
	arch::x86_64::acpi::init();
	console::kprintf("ACPI Initialised...");

	timers::init();
	console::kprintf("Timers Initialised...");

	// sleeping just for the reason of allowing the user to take a look at the beautiful ascii art,
	// and covering up the crime with the idea of "checking if sleep works" when it will always be
	// working because elapsed uses the same counter as sleep :)
	constexpr uint64_t test_duration_timer = 3;
	uint64_t total_seconds_uptime_begin = timers::total_elapsed();

	timers::sleep(test_duration_timer);

	uint64_t total_seconds_uptime_end = timers::total_elapsed();

	if ((total_seconds_uptime_end - total_seconds_uptime_begin) < test_duration_timer)
	{
		console::kprintf(ANSI_BRIGHT_RED "Timer misfunctioning"); // no its not idiot
		console::kprintf("Halting...");
		hcf();
	}

	// writing the apic literally violates geneva convention
	arch::x86_64::apic::init();
	console::kprintf("APIC Initialised...");

	interrupts::enable_all();
	console::kprintf("Enabled all interrupts...");

	// random numbers go vroom
	init_rand();
	char* method;
	switch (rand_method())
	{
		case RAND_METHOD_RDRAND: method = "RDRAND"; break;
		case RAND_METHOD_SPLITMIX64: method = "SplitMix64"; break;
		default: method = "Unknown method..."; break;
	}
	console::kprintf("Initialised RNG as %s...", method);

	pcie::init();
	console::kprintf("Initialised PCIe");

    hcf(); // How Did We Get Here?
}
