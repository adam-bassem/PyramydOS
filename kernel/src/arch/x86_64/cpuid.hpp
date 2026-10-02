#pragma once

#include <cstdint>

struct cpuid_result
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
};

namespace arch::x86_64::cpuid
{
	cpuid_result cpuid(uint32_t leaf, uint32_t subleaf);
}
