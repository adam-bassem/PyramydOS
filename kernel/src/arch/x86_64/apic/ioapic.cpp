#include "ioapic.hpp"
#include <allocator/allocator.hpp>
#include <console/console.hpp>
#include <utils.hpp>
#include <arch/x86_64/mm/vmm.hpp>

#define IOAPIC_ID	0x00
#define IOAPIC_VER	0x01
#define IOAPIC_ARB	0x02
#define IOAPIC_REDTBL(n) (0x10 + 2 * n)

struct gsi2irq_mapping
{
    uint8_t irq_src;
    uint32_t gsi;
    gsi2irq_mapping* next;
} *first_gsi2irq_mapping;

struct ioapic_entry
{
	void* base;
	uint32_t gsi_base;
	uint8_t id;
	uint8_t version;
	uint8_t max_redir_entry;
	ioapic_entry* next;
} *first_ioapic = nullptr;

namespace
{
    gsi2irq_mapping* last_gsi2irq_mapping()
    {
        gsi2irq_mapping* mapping = first_gsi2irq_mapping;

		if (!mapping)
			return nullptr;
        
        while (mapping->next)
        {
            mapping = mapping->next;
        }
        
        return mapping;
    }

    ioapic_entry* last_ioapic()
    {
    	ioapic_entry* ioapic = first_ioapic;

		if (!ioapic)
			return nullptr;
    	
    	while (ioapic->next)
    	{
    		ioapic = ioapic->next;
    	}
    	
    	return ioapic;
    }

    void write_ioapic_reg(ioapic_entry* ioapic, const uint8_t offset, const uint32_t val)
    {
    	if (!ioapic || !ioapic->base)
    	{
    		console::kprintf(ANSI_BOLD ANSI_RED "Attempted to write to a " ANSI_BOLD ANSI_PURPLE "NULL" ANSI_BOLD ANSI_RED "I/O APIC");
    		console::kprintf("Halting...");
    		hcf();
    	}
    	
    	*(volatile uint32_t*)(ioapic->base) = offset;
    	*(volatile uint32_t*)(ioapic->base + 0x10) = val;
    }

    uint32_t read_ioapic_reg(ioapic_entry* ioapic, const uint8_t offset)
    {
   		if (!ioapic || !ioapic->base)
   		{
   			console::kprintf(ANSI_BOLD ANSI_RED "Attempted to read to a " ANSI_BOLD ANSI_PURPLE "NULL" ANSI_BOLD ANSI_RED "I/O APIC");
   			console::kprintf("Halting...");
   			hcf();
   		}
   		
    	*(volatile uint32_t*)(ioapic->base) = offset;
    	return *(volatile uint32_t*)(ioapic->base + 0x10);
    }

    enum delivery_mode
    {
    	EDGE_TRIGGERED,
    	LEVEL_TRIGGERED
    };

    enum destination_mode
    {
    	PHYSICAL,
    	LOGICAL
    };

    union redirection_entry
    {
    	struct
    	{
    		uint64_t vector			: 8;
    		uint64_t delvMode		: 3;
    		uint64_t destMode		: 1;
    		uint64_t delvStatus		: 1;
    		uint64_t pinPolarity	: 1;
    		uint64_t remoteIRR		: 1;
    		uint64_t triggerMode	: 1;
    		uint64_t mask			: 1;
    		uint64_t reserved		: 39;
    		uint64_t destination	: 8;
    	};
    	struct
    	{
    		uint32_t lowerDword;
    		uint32_t upperDword;
    	};
    };
}

void arch::x86_64::ioapic::init()
{
	console::kprintf("Checking all known I/O APICs");
	int i = 0;
	ioapic_entry* entry = first_ioapic;
	
	while (entry)
	{
		console::kprintf("Parsing I/O APIC #%d...", i);
		void* virtPage = arch::x86_64::vmm::alloc_pages(1);
		arch::x86_64::vmm::mmap(entry->base, virtPage, PTE_PRESENT | PTE_WRITABLE, 1);
		entry->base = virtPage;

		console::kprintf("IOAPIC_VER = 0x%x", read_ioapic_reg(entry, IOAPIC_VER));
		console::kprintf("IOAPIC_ID = 0x%x", read_ioapic_reg(entry, IOAPIC_ID));

		entry->id = (read_ioapic_reg(entry, IOAPIC_ID) >> 24)  & 0xF0;
		entry->version = (uint8_t)read_ioapic_reg(entry, IOAPIC_VER);
		entry->max_redir_entry = (uint8_t)((read_ioapic_reg(entry, IOAPIC_VER) >> 16) + 1);

		console::kprintf("I/O APIC #%d (ID#%u):", i, entry->id);
		console::kprintf("\tVersion -> %u", entry->version);
		console::kprintf("\tRedirection Entries Count -> %u", entry->max_redir_entry);
	
		entry = entry->next;
		i++;
	}
}

void arch::x86_64::ioapic::add_ioapic(void* address, uint32_t gsi_base)
{
    console::kprintf("Added I/O APIC, address = 0x%p, gsi_base=%u", address, gsi_base);

	ioapic_entry* last = last_ioapic();
	if (!last)
	{
		last = reinterpret_cast<ioapic_entry*>(alloc::malloc(sizeof(ioapic_entry)));
		if (!last)
		{
			console::kprintf(ANSI_BOLD ANSI_RED "Out of memory... Failed to allocate memory for first IOAPIC entry");
			console::kprintf("Halting...");
			hcf();
		}
		first_ioapic = last;
		goto skip;
	}
	last->next = reinterpret_cast<ioapic_entry*>(alloc::malloc(sizeof(ioapic_entry)));
	if (!last->next)
	{
		console::kprintf(ANSI_BOLD ANSI_RED "Out of memory... Failed to allocate memory for IOAPIC entry");
		console::kprintf("Halting...");
		hcf();
	}

	last = last->next;

skip:
	last->base = address;
	last->gsi_base = gsi_base;
	last->next = nullptr;
}

void arch::x86_64::ioapic::register_iso(uint8_t bus_src, uint8_t irq_src, uint32_t gsi)
{
    console::kprintf("Registered IRQ(%u)->GSI(%u) mapping", irq_src, gsi);

    gsi2irq_mapping* last = last_gsi2irq_mapping();
    if (!last)
    {
    	last = reinterpret_cast<gsi2irq_mapping*>(alloc::malloc(sizeof(gsi2irq_mapping)));
    	if (!last)
    	{
    		console::kprintf(ANSI_BOLD ANSI_RED "Out of memory... Failed to allocate memory for first GSI to IRQ mapping entry");
    		console::kprintf("Halting...");
    		hcf();
    	}
    	first_gsi2irq_mapping = last;
    	goto skip;
    }
    last->next = reinterpret_cast<gsi2irq_mapping*>(alloc::malloc(sizeof(gsi2irq_mapping)));
    if (!last->next)
    {
    	console::kprintf(ANSI_BOLD ANSI_RED "Out of memory... Failed to allocate memory for GSI to IRQ mapping entry");
    	console::kprintf("Halting...");
    	hcf();
    }
    
    last = last->next;

skip:
    last->irq_src = irq_src;
    last->gsi = gsi;
    last->next = nullptr;
}
