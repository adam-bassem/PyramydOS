#include "timers.hpp"
#include <arch/x86_64/timers/hpet.hpp>
#include <console/console.hpp>
#include <interrupts/ints.hpp>

void timers::init()
{
	arch::x86_64::timers::hpet::init();
}

void timers::sleep_us(uint64_t time_us)
{
	arch::x86_64::timers::hpet::sleep_us(time_us);
}

void timers::sleep_ms(uint64_t time_ms)
{
	arch::x86_64::timers::hpet::sleep_ms(time_ms);
}

void timers::sleep(uint64_t time)
{
	arch::x86_64::timers::hpet::sleep(time);
}

uint64_t timers::total_us_elapsed()
{
	return arch::x86_64::timers::hpet::total_us_elapsed();

	return 0;
}

uint64_t timers::total_ms_elapsed()
{
	return arch::x86_64::timers::hpet::total_ms_elapsed();

	return 0;
}

uint64_t timers::total_elapsed()
{
	return arch::x86_64::timers::hpet::total_elapsed();

	return 0;
}

extern "C" [[gnu::interrupt]]
void timer_periodic_interrupt(void*)
{
    interrupts::send_eoi();
}
