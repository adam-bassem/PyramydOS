#include "lapic.hpp"
#include <arch/x86_64/idt/idt.hpp>
#include <arch/x86_64/msr.hpp>
#include <arch/x86_64/timers/hpet.hpp>
#include <timers/timers.hpp>

#define APIC_TIMER_VECTOR 0x32
#define APIC_LVT_INT_MASKED (1ULL << 16)
#define APIC_LVT_TIMER_MODE_PERIODIC (1ULL << 17)

#define APIC_SOFTWARE_ENABLE (1ULL << 8)
#define APIC_ENABLE (1ULL << 11)
#define X2APIC_ENABLE (1ULL << 10)

#define IA32_APIC_BASE 0x1B
#define X2APIC_MSR_BASE 0x800

extern "C" void timer_int();

extern "C" [[gnu::interrupt]] void lapic_spurious_vector(void*)
{
}

namespace arch::x86_64::lapic
{
	lapic lapic_bsp;

	void lapic::init(void* vbase, bool use_x2apic)
	{
		base = vbase;
		x2apic = use_x2apic;

		uint64_t apic_base = msr::read(IA32_APIC_BASE);

		apic_base |= APIC_ENABLE;

		if (x2apic)
			apic_base |= X2APIC_ENABLE;

		msr::write(IA32_APIC_BASE, apic_base);
	}

	uint32_t lapic::read(uint32_t reg) const
	{
		if (x2apic)
			return msr::read(X2APIC_MSR_BASE + (reg >> 4));

		return *reinterpret_cast<volatile uint32_t*>(
			reinterpret_cast<uint64_t>(base) + reg
		);
	}

	void lapic::write(uint32_t reg, uint32_t value)
	{
		if (x2apic)
			return msr::write(X2APIC_MSR_BASE + (reg >> 4), value);

		*reinterpret_cast<volatile uint32_t*>(
			reinterpret_cast<uint64_t>(base) + reg
		) = value;
	}

	void lapic::send_eoi()
	{
		write(REG_EOI, 0);
	}

	void lapic::enable()
	{
		arch::x86_64::idt::set_handler((void*)lapic_spurious_vector, 0xFF, ENTRY_PRESENT | ENTRY_DPL0 | ENTRY_INTERRUPT_GATE);
		arch::x86_64::idt::set_handler((void*)timer_int, APIC_TIMER_VECTOR, ENTRY_PRESENT | ENTRY_DPL0 | ENTRY_INTERRUPT_GATE);

		write(REG_TPR, 0);
		write(REG_SPURIOUS, APIC_SOFTWARE_ENABLE | 0xFF);

		write(REG_TIMER_DIVIDE, 0x3);
		write(REG_LVT_TIMER, APIC_LVT_INT_MASKED);
		write(REG_TIMER_INITIAL_COUNT, 0xFFFFFFFF);

		arch::x86_64::timers::hpet::sleep_ms(100);

		uint32_t tickIn100ms = 0xFFFFFFFF - read(REG_TIMER_CURRENT_COUNT);
		uint32_t tickIn1ms = tickIn100ms / 100;

		write(REG_LVT_TIMER, APIC_TIMER_VECTOR | APIC_LVT_TIMER_MODE_PERIODIC);
		write(REG_TIMER_DIVIDE, 0x3);
		write(REG_TIMER_INITIAL_COUNT, tickIn1ms);
	}
}
