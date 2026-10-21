#include "ints.hpp"

#include <arch/x86_64/irq/irq.hpp>

void interrupts::set_irq(uint32_t irq, handler_t handler)
{
	arch::x86_64::irq::set_irq(irq, (void*)handler);
}

void interrupts::clear_irq(uint32_t irq)
{
	arch::x86_64::irq::clear_irq(irq);
}

void interrupts::mask_irq(uint32_t irq)
{
	arch::x86_64::irq::mask_irq(irq);
}

void interrupts::unmask_irq(uint32_t irq)
{
	arch::x86_64::irq::unmask_irq(irq);
}

void interrupts::send_eoi()
{
	arch::x86_64::irq::send_eoi();
}

void interrupts::enable_all()
{
	asm ("sti");
}

void interrupts::disable_all()
{
	asm ("cli");
}
