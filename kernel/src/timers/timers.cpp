#include "timers.hpp"

#if defined(__x86_64__)
#	include <arch/x86_64/timers/hpet.hpp>
#endif

void timers::init()
{
#if defined(__x86_64__)
	arch::x86_64::timers::hpet::init();
#endif
}

void timers::sleep_us(uint64_t time_us)
{
#if defined(__x86_64__)
	arch::x86_64::timers::hpet::sleep_us(time_us);
#endif	
}

void timers::sleep_ms(uint64_t time_ms)
{
#if defined(__x86_64__)
	arch::x86_64::timers::hpet::sleep_ms(time_ms);
#endif
}

void timers::sleep(uint64_t time)
{
#if defined(__x86_64__)
	arch::x86_64::timers::hpet::sleep(time);
#endif
}

uint64_t timers::total_us_elapsed()
{
#if defined(__x86_64__)
	return arch::x86_64::timers::hpet::total_us_elapsed();
#endif

	return 0;
}

uint64_t timers::total_ms_elapsed()
{
#if defined(__x86_64__)
	return arch::x86_64::timers::hpet::total_ms_elapsed();
#endif

	return 0;
}

uint64_t timers::total_elapsed()
{
#if defined(__x86_64__)
	return arch::x86_64::timers::hpet::total_elapsed();
#endif

	return 0;
}
