#pragma once

#include <cstdint>

namespace interrupts
{
	using handler_t = void (*)(uint32_t irq);

	void set_irq(uint32_t irq, handler_t handler);
	void clear_irq(uint32_t irq);
	void mask_irq(uint32_t irq);
	void unmask_irq(uint32_t irq);
	void send_eoi(uint32_t irq);
	void enable_all();
	void disable_all();
}
