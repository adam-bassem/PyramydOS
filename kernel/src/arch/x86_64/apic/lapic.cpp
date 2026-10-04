#include "lapic.hpp"
#include <arch/x86_64/idt/idt.hpp>

extern "C" __attribute__((interrupt)) void lapic_spurious_vector(void*)
{
}

namespace arch::x86_64::lapic
{
	lapic lapic_bsp;

	void lapic::init(void* vbase)
	{
		base = vbase;
	}

	uint32_t lapic::read(uint32_t reg) const
	{
		return *reinterpret_cast<volatile uint32_t*>(reinterpret_cast<uint64_t>(base) + reg);
	}

	void lapic::write(uint32_t reg, uint32_t value)
	{
		*reinterpret_cast<volatile uint32_t*>(reinterpret_cast<uint64_t>(base) + reg) = value;
	}

	void lapic::send_eoi()
	{
		write(REG_EOI, 0);
	}

	void lapic::enable()
	{
		arch::x86_64::idt::set_handler((void*)lapic_spurious_vector, 0xFF, ENTRY_PRESENT | ENTRY_DPL0 | ENTRY_INTERRUPT_GATE);
	
		write(REG_TPR, 0);
		write(REG_SPURIOUS, 0x100 | 0xFF);
	}
}
