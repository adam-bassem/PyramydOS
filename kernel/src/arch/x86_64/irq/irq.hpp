#pragma once

#include <cstdint>

namespace arch::x86_64::irq
{
	void set_irq(uint32_t irq, void* handler);
	void clear_irq(uint32_t irq);
	void mask_irq(uint32_t irq);
	void unmask_irq(uint32_t irq);
	void dispatch_irq(uint32_t irq);
	void send_eoi();
}
