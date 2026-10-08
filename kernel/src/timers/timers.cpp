#include "timers.hpp"

#if defined(__x86_64__)
#	include <arch/x86_64/timers/hpet.hpp>
#endif
#include <console/console.hpp>
#include <interrupts/ints.hpp>

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

#if defined (__x86_64__)
struct [[gnu::packed]] timer_int_ctx
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
};
#endif

extern "C" timer_int_ctx* timer_periodic_interrupt(timer_int_ctx* ctx)
{
	interrupts::send_eoi(0);
	return ctx;
}
