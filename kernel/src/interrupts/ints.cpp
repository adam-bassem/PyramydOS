#include "ints.hpp"

#if defined(__x86_64__)
#	include <arch/x86_64/irq/irq.hpp>
#endif

void interrupts::set_irq(uint32_t irq, handler_t handler)
{
#if defined(__x86_64__)
	arch::x86_64::irq::set_irq(irq, (void*)handler);
#endif	
}

void interrupts::clear_irq(uint32_t irq)
{
#if defined(__x86_64__)
	arch::x86_64::irq::clear_irq(irq);
#endif		
}

void interrupts::mask_irq(uint32_t irq)
{
#if defined(__x86_64__)
	arch::x86_64::irq::mask_irq(irq);
#endif	
}

void interrupts::unmask_irq(uint32_t irq)
{
#if defined(__x86_64__)
	arch::x86_64::irq::unmask_irq(irq);
#endif	
}

void interrupts::send_eoi(uint32_t irq)
{
#if defined(__x86_64__)
	(void)irq;
	arch::x86_64::irq::send_eoi();
#endif	
}

void interrupts::enable_all()
{
#if defined(__x86_64__)
	asm ("sti");
#endif	
}

void interrupts::disable_all()
{
#if defined(__x86_64__)
	asm ("cli");
#endif	
}
