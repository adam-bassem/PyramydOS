#include "apic.hpp"
#include "ioapic.hpp"
#include "lapic.hpp"
#include <arch/x86_64/acpi/acpi.hpp>
#include <arch/x86_64/cpuid.hpp>
#include <console/console.hpp>
#include <utils.hpp>
#include <arch/x86_64/io.hpp>
#include <arch/x86_64/mm/vmm.hpp>
#include <arch/x86_64/msr.hpp>

#define ENTRY_TYPE_PROCESSOR_LAPIC		0
#define ENTRY_TYPE_IOAPIC				1
#define ENTRY_TYPE_IOAPIC_ISO			2
#define ENTRY_TYPE_IOAPIC_NMI_INT_SRC	3
#define ENTRY_TYPE_LAPIC_NMI			4
#define ENTRY_TYPE_LAPIC_ADDRESS_OVERRIDE 5
#define ENTRY_TYPE_PROCESSOR_LOCAL_X2APIC 6

#define IA32_APIC_BASE 0x1B
#define APIC_ENABLE    (1ULL << 11)
#define X2APIC_ENABLE  (1ULL << 10)

// FLAGS for entries type 2 (IOAPIC ISO), 3 (IOAPIC NMI INT SRC) and 4 (LAPIC NMI)
/*
 * Offset 0
 *   Length 2
 *     Polarity:
 *       0b00 - No override
 *       0b01 - Active high override
 *       0b10 - Reserved
 *       0b11 - Active low override
 * Offset 2
 *   Length 2
 *     Trigger mode
 *       0b00 - No override
 *       0b01 - Edge triggered
 *       0b10 - Reserved
 *       0b11 - Level triggered
 * Offset 4
 *   Length 12
 *     Reserved
 */

namespace
{
	struct [[gnu::packed]] madt_entry
	{
		uint8_t entry_type;
		uint8_t record_length;

		union
		{
			struct [[gnu::packed]]
			{
				uint8_t acpi_processor_id;
				uint8_t apic_id;
				uint32_t flags;
			} processor_lapic;

			struct [[gnu::packed]]
			{
				uint8_t ioapic_id;
				uint8_t reserved;
				uint32_t ioapic_address;
				uint32_t gsi_base;
			} ioapic;

			struct [[gnu::packed]]
			{
				uint8_t bus_source;
				uint8_t irq_source;
				uint32_t gsi;
				uint16_t flags;
			} ioapic_iso;

			struct [[gnu::packed]]
			{
				uint8_t nmi_source;
				uint8_t reserved;
				uint16_t flags;
				uint32_t gsi;
			} ioapic_nmi_int_src;

			struct [[gnu::packed]]
			{
				uint8_t acpi_processor_id; // 0xFF for all
				uint16_t flags;
				uint8_t lintx;
			} lapic_nmi;

			struct [[gnu::packed]]
			{
				uint16_t reserved;
				uint64_t lapic_address;
			} lapic_address_override;

			struct [[gnu::packed]]
			{
				uint16_t reserved;
				uint32_t processor_local_x2apic_id;
				uint32_t flags;
				uint32_t acpi_id;
			} processor_local_x2apic;
		};
	};

	struct [[gnu::packed]] madt_table
	{
		acpi_sdt_header header;

		uint32_t lapic_address;
		uint32_t flags;
	} *madt;

	uint32_t lapic_bsp;
	void* lapic_address;
	bool x2apic;
}

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

	r = arch::x86_64::cpuid::cpuid(1, 0);
	x2apic = r.ecx & (1ULL << 21);
	if (x2apic) console::kprintf("x2APIC supported");

	uint64_t apic_base = arch::x86_64::msr::read(IA32_APIC_BASE);

	apic_base |= APIC_ENABLE;
	
	if (x2apic)
	{
		apic_base |= X2APIC_ENABLE;
	}
	
	arch::x86_64::msr::write(IA32_APIC_BASE, apic_base);

	arch::x86_64::io::outb(0x21, 0xFF);
	arch::x86_64::io::outb(0xA1, 0xFF);

	madt = reinterpret_cast<madt_table*>(arch::x86_64::acpi::get_table("APIC"));
	if (!madt)
	{
		console::kprintf(ANSI_BOLD ANSI_RED "Could not find MADT (\"APIC\") table...");
		console::kprintf("Halting...");
		hcf();
	}
	console::kprintf("Acquired MADT table from ACPI");

	lapic_bsp = mp_request.response->bsp_lapic_id;
	console::kprintf("Acquired BSP LAPIC ID: 0x%x", lapic_bsp);
	lapic_address = reinterpret_cast<void*>(madt->lapic_address);

	uint64_t cursor = reinterpret_cast<uint64_t>(reinterpret_cast<uint64_t>(madt) + sizeof(madt_table));
	uint64_t end = reinterpret_cast<uint64_t>(reinterpret_cast<uint64_t>(madt) + madt->header.length);

	while (cursor < end)
	{
		madt_entry* entry = reinterpret_cast<madt_entry*>(cursor);

		switch (entry->entry_type)
		{
			case ENTRY_TYPE_PROCESSOR_LAPIC:
			{
				console::kprintf("Found Local APIC entry:");
				console::kprintf(
					ANSI_GREEN "\tACPI Processor ID:" ANSI_CYAN " %u\t" ANSI_GREEN "APIC ID:" ANSI_CYAN " %u\t" ANSI_GREEN "Flags:" ANSI_CYAN " 0x%x",
					entry->processor_lapic.acpi_processor_id, entry->processor_lapic.apic_id, entry->processor_lapic.flags
				);
			
				break;
			}

			case ENTRY_TYPE_IOAPIC:
			{
				console::kprintf("Found I/O APIC entry:");
				console::kprintf(
					ANSI_GREEN "\tI/O APIC ID:" ANSI_CYAN " 0x%x" ANSI_GREEN "\tI/O APIC Address:" ANSI_CYAN " 0x%p\t" ANSI_GREEN "GSI Base:" ANSI_CYAN " %u",
					entry->ioapic.ioapic_id, entry->ioapic.ioapic_address, entry->ioapic.gsi_base
				);

                arch::x86_64::ioapic::add_ioapic((void*)entry->ioapic.ioapic_address, entry->ioapic.gsi_base);
			
				break;
			}

			case ENTRY_TYPE_IOAPIC_ISO:
			{
				console::kprintf("Found I/O APIC Interrupt Source Override entry:");
				console::kprintf(ANSI_GREEN "\t" ANSI_GREEN "Bus Source:" ANSI_CYAN " %u\t" ANSI_GREEN "IRQ Source:" ANSI_CYAN " %u\t" ANSI_GREEN "GSI:" ANSI_CYAN " %u\t" ANSI_GREEN "Flags:" ANSI_CYAN " 0x%x", entry->ioapic_iso.bus_source, entry->ioapic_iso.irq_source, entry->ioapic_iso.gsi, entry->ioapic_iso.flags);
			
                arch::x86_64::ioapic::register_iso(entry->ioapic_iso.bus_source, entry->ioapic_iso.irq_source, entry->ioapic_iso.gsi);

				break;
			}

			case ENTRY_TYPE_IOAPIC_NMI_INT_SRC:
			{
				console::kprintf("Found I/O APIC Non-Maskable Interrupt Source entry:");
				console::kprintf(ANSI_GREEN "\tNMI Source:" ANSI_CYAN " %u\t" ANSI_GREEN "Flags:" ANSI_CYAN " 0x%x\t" ANSI_GREEN "GSI:" ANSI_CYAN " %u", entry->ioapic_nmi_int_src.nmi_source, entry->ioapic_nmi_int_src.flags, entry->ioapic_nmi_int_src.gsi);
			
				break;
			}

			case ENTRY_TYPE_LAPIC_NMI:
			{
				console::kprintf("Found Local APIC Non-Maskable Interrupts entry:");
				console::kprintf(ANSI_GREEN "\tACPI Processor ID:" ANSI_CYAN " 0x%x\t" ANSI_GREEN "Flags:" ANSI_CYAN " 0x%x\t" ANSI_GREEN "LINT#:" ANSI_CYAN " %u", entry->lapic_nmi.acpi_processor_id, entry->lapic_nmi.flags, entry->lapic_nmi.lintx);
			
				break;
			}

			case ENTRY_TYPE_LAPIC_ADDRESS_OVERRIDE:
			{
				console::kprintf("Found Local APIC Address Override entry:");
				console::kprintf(ANSI_GREEN "\tLAPIC Address:" ANSI_CYAN " 0x%p", entry->lapic_address_override.lapic_address);
				lapic_address = reinterpret_cast<void*>(entry->lapic_address_override.lapic_address);
			
				break;
			}

			case ENTRY_TYPE_PROCESSOR_LOCAL_X2APIC:
			{
				console::kprintf("Found Processor Local x2APIC entry:");
				console::kprintf(ANSI_GREEN "\tProcessor Local x2APIC ID:" ANSI_CYAN " %u\t" ANSI_GREEN "Flags:" ANSI_CYAN " 0x%x\t" ANSI_GREEN "ACPI ID:" ANSI_CYAN " %u", entry->processor_local_x2apic.processor_local_x2apic_id, entry->processor_local_x2apic.flags, entry->processor_local_x2apic.acpi_id);
			
				break;
			}
		}

		cursor += entry->record_length;
	}

	void* lapic_addr_virt = arch::x86_64::vmm::alloc_pages(1);
	arch::x86_64::vmm::mmap(lapic_address, lapic_addr_virt, PTE_PRESENT | PTE_WRITABLE, 1);

	arch::x86_64::lapic::lapic_bsp.init(lapic_addr_virt, x2apic);
	arch::x86_64::lapic::lapic_bsp.enable();
	console::kprintf("Initialised Local APIC");

	arch::x86_64::ioapic::init();
	console::kprintf("I/O APIC Initialised...");
}
