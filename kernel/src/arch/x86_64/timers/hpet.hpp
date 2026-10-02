#pragma once

#include <cstdint>

namespace arch::x86_64::timers::hpet
{
	void init();
	void sleep_us(uint64_t us_time);
	void sleep_ms(uint64_t ms_time);
	void sleep(uint64_t time);
	
	uint64_t total_timer_ticks();
	uint64_t total_us_elapsed();
	uint64_t total_ms_elapsed();
	uint64_t total_elapsed();
}
