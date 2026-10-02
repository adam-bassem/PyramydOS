#pragma once

#include <cstdint>

namespace timers
{
	void init();
	void sleep_us(uint64_t time_us);
	void sleep_ms(uint64_t time_ms);
	void sleep(uint64_t time);

	uint64_t total_us_elapsed();
	uint64_t total_ms_elapsed();
	uint64_t total_elapsed();
}
