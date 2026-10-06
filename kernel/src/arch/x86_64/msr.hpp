#pragma once

#include <cstdint>

namespace arch::x86_64::msr
{
	uint64_t read(uint32_t msr);
	void write(uint32_t msr, uint64_t value);
}
