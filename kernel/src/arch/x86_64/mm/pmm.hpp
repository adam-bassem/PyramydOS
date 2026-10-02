#pragma once

#include <cstdint>

namespace arch::x86_64::pmm
{
	void init();
	void* alloc_pages(uint64_t npages);
	void free_pages(void* ptr, uint64_t npages);
	void* malloc(uint64_t nbytes);
	void free(void* ptr, uint64_t nbytes);
	uint64_t free_pages_count();
	uint64_t total_pages_count();
}
