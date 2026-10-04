#pragma once

#include <stdint.h>

#define REG_ID 0x20
#define REG_TPR 0x80
#define REG_EOI 0xB0
#define REG_SPURIOUS 0xF0
#define REG_ISR_BASE 0x100

namespace arch::x86_64::lapic
{
	class lapic
	{
	public:
		void init(void* vbase);
		uint32_t read(uint32_t reg) const;
		void write(uint32_t reg, uint32_t value);
		void send_eoi();
		void enable();

	private:
		void* base = nullptr;
	};

	extern lapic lapic_bsp;
}
