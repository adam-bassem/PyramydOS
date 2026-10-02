#include "cpuid.hpp"

cpuid_result arch::x86_64::cpuid::cpuid(uint32_t leaf, uint32_t subleaf)
{
	cpuid_result r;

	asm volatile
	(
		"cpuid"
		: "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
		: "a"(leaf), "c"(subleaf)	
	);

	return r;
}
