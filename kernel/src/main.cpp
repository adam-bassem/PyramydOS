#include "utils.hpp"

#if defined (__x86_64__)
#	include <arch/x86_64/gdt/gdt.hpp>
#	include <arch/x86_64/idt/idt.hpp>
#	include <arch/x86_64/mm/pmm.hpp>
#	include <arch/x86_64/mm/vmm.hpp>
#	include	<arch/x86_64/acpi/acpi.hpp>
#	include <arch/x86_64/io.hpp>
#	include <arch/x86_64/cpuid.hpp>
#	include <arch/x86_64/apic/apic.hpp>
#	include <arch/x86_64/apic/ioapic.hpp>
#endif

#include <console/console.hpp>
#include <allocator/allocator.hpp>
#include <timers/timers.hpp>

extern "C" void kmain() {
    pre_kernel();

	void* heap_base;

#if defined (__x86_64__)
	arch::x86_64::gdt::init();
	arch::x86_64::idt::init();
	arch::x86_64::pmm::init();
	arch::x86_64::vmm::init();

	heap_base = arch::x86_64::vmm::alloc_pages(1);
#endif

	alloc::init(heap_base);

	console::init();
	console::swap_ctx(0);

	console::kprintf("***************************");
	console::kprintf("***    PyramydKernel    ***");
	console::kprintf("***************************");

#if defined(__x86_64__)
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

	console::kprintf(
		"CPU: %s%s%s",
		vendor,
		has_brand ? " - " : "",
		has_brand ? brand : ""
	);

	console::kprintf("PMM: %llu pages", ram_pages);

	console::print_timestamp();
	printf("RAM: %llu", ram_gib);

	if (has_decimal)
		printf(".%u", (uint32_t)ram_decimal);

	printf(" GiB\r\n" ANSI_RESET);
#endif

	console::kprintf("HHDM offset: 0x%llx", hhdm_request.response->offset);

#if defined (__x86_64__)
	console::kprintf("GDT Initialised...");
	console::kprintf("IDT Initialised...");
	console::kprintf("Physical Memory Manager Initialised...");
	console::kprintf("Virtual Memory Manager Initialised...");
#endif

	console::kprintf("Allocator Initialised...");

#if defined (__x86_64__)
	arch::x86_64::acpi::init();
	console::kprintf("ACPI Initialised...");
#endif

	timers::init();
	console::kprintf("Timers Initialised...");

#if defined (__x86_64__)
	arch::x86_64::apic::init();
	console::kprintf("APIC Initialised...");

	arch::x86_64::ioapic::init();
	console::kprintf("I/O APIC Initialised...");
#endif

    hcf();
}
