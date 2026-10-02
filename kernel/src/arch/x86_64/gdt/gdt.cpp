#include "gdt.hpp"

namespace
{

	alignas(16) arch::x86_64::gdt::tss tss{};

	constexpr uint64_t null_entry = 0x0000000000000000ULL;

	constexpr uint64_t kernel_code = 0x00AF9A000000FFFFULL;
	constexpr uint64_t kernel_data = 0x00CF92000000FFFFULL;

	constexpr uint64_t user_code   = 0x00AFFA000000FFFFULL;
	constexpr uint64_t user_data   = 0x00CFF2000000FFFFULL;

	uint64_t tss_low  = 0;
	uint64_t tss_high = 0;

	uint64_t gdt_entries[]
	{
		null_entry,
		kernel_code,
		kernel_data,
		user_code,
		user_data,
		0,
		0
	};

	void encode_tss()
	{
		const uint64_t base = reinterpret_cast<uint64_t>(&tss);
		const uint64_t limit = sizeof(arch::x86_64::gdt::tss) - 1;

		constexpr uint8_t access = 0x89;
		constexpr uint8_t flags = 0x00;

		tss_low =
			(limit & 0xFFFFULL)
			| ((base & 0xFFFFFFULL) << 16)
			| (static_cast<uint64_t>(access) << 40)
			| (((limit >> 16) & 0xFULL) << 48)
			| (static_cast<uint64_t>(flags) << 52)
			| (((base >> 24) & 0xFFULL) << 56);

		tss_high = (base >> 32) & 0xFFFFFFFFULL;

		gdt_entries[5] = tss_low;
		gdt_entries[6] = tss_high;
	}

}

void arch::x86_64::gdt::init()
{
	encode_tss();

	static const gdtr gdtr_ptr
	{
		.limit = sizeof(gdt_entries) - 1,
		.base  = reinterpret_cast<uint64_t>(&gdt_entries)
	};

	asm volatile
	(
		"lgdt %0\n"
		"pushq $0x08\n"
		"lea 1f(%%rip), %%rax\n"
		"pushq %%rax\n"
		"lretq\n"
		"1:\n"
		"mov $0x10, %%ax\n"
		"mov %%ax, %%ds\n"
		"mov %%ax, %%es\n"
		"mov %%ax, %%fs\n"
		"mov %%ax, %%gs\n"
		"mov %%ax, %%ss\n"
		"ltr %1\n"
		:
		: "m" (gdtr_ptr), "r" (static_cast<uint16_t>(0x28))
		: "rax", "memory"
	);
}
