#pragma once

#include <cstdint>

namespace arch::x86_64::gdt
{

	struct [[gnu::packed]] gdtr
	{
		uint16_t limit;
		uint64_t base;
	};

	struct [[gnu::packed]] tss
	{
		uint32_t reserved0;
		uint64_t rsp[3];
		uint64_t reserved1;
		uint64_t ist[7];
		uint64_t reserved2;
		uint16_t reserved3;
		uint16_t iopb_offset;
	};

	static_assert(sizeof(tss) >= 104);

	void init();

}
