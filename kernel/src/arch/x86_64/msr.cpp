#include "msr.hpp"

uint64_t arch::x86_64::msr::read(uint32_t msr)
{
	uint32_t lo, hi;

	asm volatile (
		"rdmsr"
		: "=a"(lo), "=d"(hi)
		: "c"(msr)
	);

	return ((uint64_t)hi << 32) | lo;
}

void arch::x86_64::msr::write(uint32_t msr, uint64_t value)
{
	uint32_t lo = value & 0xFFFFFFFF;
	uint32_t hi = value >> 32;

	asm volatile (
		"wrmsr"
		:
		: "a"(lo), "d"(hi), "c"(msr)
	);
}
