#pragma once

#include <utils.hpp>
#include <cstdint>

constexpr uint8_t idt_entry_present = 1 << 7;

constexpr uint8_t idt_entry_dpl0 = 0 << 5;
constexpr uint8_t idt_entry_dpl1 = 1 << 5;
constexpr uint8_t idt_entry_dpl2 = 2 << 5;
constexpr uint8_t idt_entry_dpl3 = 3 << 5;

constexpr uint8_t idt_entry_interrupt_gate = 0xE;
constexpr uint8_t idt_entry_trap_gate = 0xF;

constexpr uint32_t num_ints = 256;

extern "C" void* exception_stub_table[32];

namespace arch::x86_64::idt
{
	struct [[gnu::packed]] idt_entry
	{
		uint16_t off_low;
		uint16_t selector;
		uint8_t ist;
		uint8_t attr;
		uint16_t off_mid;
		uint32_t off_high;
		uint32_t zero;
	};

	struct [[gnu::packed]] idtr
	{
		uint16_t limit;
		uint64_t base;
	};

	void init();

	void set_handler(void* handler, uint8_t vector, uint8_t flags);

	uint8_t allocate_entry();
	void free_entry(uint8_t entry);
}
