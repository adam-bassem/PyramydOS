#include "lapic.hpp"
#include <arch/x86_64/idt/idt.hpp>
#include <arch/x86_64/msr.hpp>
#include <arch/x86_64/timers/hpet.hpp>
#include <timers/timers.hpp>

constexpr uint32_t tick_freq_hz = 10/*hz*/;

constexpr uint32_t apic_timer_vector = 0x32;
constexpr uint64_t apic_lvt_int_masked = 1ULL << 16;
constexpr uint64_t apic_lvt_timer_mode_periodic = 1ULL << 17;

constexpr uint64_t apic_software_enable = 1ULL << 8;
constexpr uint64_t apic_enable = 1ULL << 11;
constexpr uint64_t x2apic_enable = 1ULL << 10;

constexpr uint32_t ia32_apic_base = 0x1B;
constexpr uint32_t x2apic_msr_base = 0x800;

extern "C" [[gnu::interrupt]] void timer_periodic_interrupt(void*);

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

		uint64_t apic_base = msr::read(ia32_apic_base);

		apic_base |= apic_enable;

		if (x2apic)
			apic_base |= x2apic_enable;

		msr::write(ia32_apic_base, apic_base);
	}

	uint32_t lapic::read(uint32_t reg) const
	{
		if (x2apic)
			return msr::read(x2apic_msr_base + (reg >> 4));

		return *reinterpret_cast<volatile uint32_t*>(
			reinterpret_cast<uint64_t>(base) + reg
		);
	}

	void lapic::write(uint32_t reg, uint32_t value)
	{
		if (x2apic)
			return msr::write(x2apic_msr_base + (reg >> 4), value);

		*reinterpret_cast<volatile uint32_t*>(
			reinterpret_cast<uint64_t>(base) + reg
		) = value;
	}

	void lapic::send_eoi()
	{
		write(reg_eoi, 0);
	}

	void lapic::enable()
	{
		arch::x86_64::idt::set_handler((void*)lapic_spurious_vector, 0xFF, idt_entry_present | idt_entry_dpl0 | idt_entry_interrupt_gate);
		arch::x86_64::idt::set_handler((void*)timer_periodic_interrupt, apic_timer_vector, idt_entry_present | idt_entry_dpl0 | idt_entry_interrupt_gate);

		// some initialisation stuff
		write(reg_tpr, 0);
		write(reg_spurious, apic_software_enable | 0xFF);

		// calibration stuff

		write(reg_timer_divide, 0x3);
		write(reg_lvt_timer, apic_lvt_int_masked);
		write(reg_timer_initial_count, 0xFFFFFFFF);

		arch::x86_64::timers::hpet::sleep_ms(100);

		uint32_t ticks = read(reg_timer_current_count);
		uint32_t ticks_in_100ms = 0xFFFFFFFF - ticks;
		uint32_t ticks_per_sec = ticks_in_100ms * 10;

		write(reg_lvt_timer, apic_timer_vector | apic_lvt_timer_mode_periodic);
		write(reg_timer_divide, 0x3);
		write(reg_timer_initial_count, ticks_per_sec / tick_freq_hz);
	}
}
