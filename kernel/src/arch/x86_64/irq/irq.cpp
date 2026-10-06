#include "irq.hpp"
#include <arch/x86_64/idt/idt.hpp>
#include <arch/x86_64/apic/ioapic.hpp>
#include <arch/x86_64/apic/lapic.hpp>
#include <console/console.hpp>
#include <utils.hpp>
#include <allocator/allocator.hpp>

namespace
{
	struct irq_entry
	{
		uint32_t irq;
		uint32_t gsi;
		uint8_t idt_entry;

		irq_entry* prev;
		irq_entry* next;
	};

	irq_entry* first_irq_entry = nullptr;

	irq_entry* get_last_entry()
	{
		irq_entry* entry = first_irq_entry;

		if (!entry)
		{
			return nullptr;
		}

		while (entry->next)
		{
			entry = entry->next;
		}

		return entry;
	}

	irq_entry* find_irq_entry(uint32_t irq)
	{
		for (irq_entry* e = first_irq_entry; e; e = e->next)
		{
			if (e->irq == irq)
			{
				return e;
			}
		}

		return nullptr;
	}

	void push_irq_entry(irq_entry entry)
	{
		irq_entry* last = get_last_entry();

		irq_entry* e = reinterpret_cast<irq_entry*>(alloc::malloc(sizeof(irq_entry)));
		if (!e)
		{
			console::kprintf(ANSI_BOLD ANSI_RED "Out of memory... Failed to allocate IRQ entry");
			console::kprintf("Halting...");
			hcf();
		}

		e->irq = entry.irq;
		e->gsi = entry.gsi;
		e->idt_entry = entry.idt_entry;
		e->prev = last;
		e->next = nullptr;

		if (last)
		{
			last->next = e;
		}
		else
		{
			first_irq_entry = e;
		}
	}

	void purge_irq_entry(irq_entry* entry)
	{
		if (entry->prev)
		{
			entry->prev->next = entry->next;
		}
		else
		{
			first_irq_entry = entry->next;
		}

		if (entry->next)
		{
			entry->next->prev = entry->prev;
		}

		alloc::free(entry);
	}
}

void arch::x86_64::irq::set_irq(uint32_t irq, void* handler)
{
	console::kprintf("set_irq: irq=%u", irq);

	uint8_t entry = arch::x86_64::idt::allocate_entry();
	console::kprintf("set_irq: vector=%u", entry);

	irq_entry e;
	e.irq = irq;
	e.gsi = arch::x86_64::ioapic::get_gsi_for_irq(irq);
	e.idt_entry = entry;
	console::kprintf("set_irq: gsi=%u", e.gsi);

	push_irq_entry(e);

	arch::x86_64::idt::set_handler(handler, e.idt_entry, ENTRY_PRESENT | ENTRY_DPL0 | ENTRY_INTERRUPT_GATE);
	arch::x86_64::ioapic::set_redir_entry(e.gsi, e.idt_entry, true);
}

void arch::x86_64::irq::clear_irq(uint32_t irq)
{
	irq_entry* e = find_irq_entry(irq);
	if (!e)
	{
		return;
	}

	arch::x86_64::ioapic::set_redir_entry(e->gsi, 0, true);
	arch::x86_64::idt::set_handler(nullptr, e->idt_entry, 0);
	purge_irq_entry(e);
}

void arch::x86_64::irq::mask_irq(uint32_t irq)
{
	irq_entry* e = find_irq_entry(irq);
	if (e)
	{
		arch::x86_64::ioapic::set_redir_entry(e->gsi, e->idt_entry, true);
	}
}

void arch::x86_64::irq::unmask_irq(uint32_t irq)
{
	irq_entry* e = find_irq_entry(irq);
	if (e)
	{
		arch::x86_64::ioapic::set_redir_entry(e->gsi, e->idt_entry, false);
	}
}

void arch::x86_64::irq::send_eoi()
{
	arch::x86_64::lapic::lapic_bsp.send_eoi();
}
