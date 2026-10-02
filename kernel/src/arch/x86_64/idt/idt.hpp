#pragma once

#include <utils.hpp>
#include <cstdint>

#define ENTRY_PRESENT        (uint8_t)(1 << 7)

#define ENTRY_DPL0           (uint8_t)(0 << 5)
#define ENTRY_DPL1           (uint8_t)(1 << 5)
#define ENTRY_DPL2           (uint8_t)(2 << 5)
#define ENTRY_DPL3           (uint8_t)(3 << 5)

#define ENTRY_INTERRUPT_GATE (uint8_t)0xE
#define ENTRY_TRAP_GATE      (uint8_t)0xF

#define NUM_INTS 256

extern "C" void* exception_stub_table[22];

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
}
