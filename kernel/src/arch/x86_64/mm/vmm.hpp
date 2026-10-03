#pragma once

#include <cstdint>

#define PTE_PRESENT   (1ULL << 0)
#define PTE_WRITABLE  (1ULL << 1)
#define PTE_USER      (1ULL << 2)
#define PTE_HUGE      (1ULL << 7)
#define PTE_NX        (1ULL << 63)
#define HUGE_2M_MASK  (0x000FFFFFFFE00000ULL)
#define HUGE_1G_MASK  (0x000FFFFFC0000000ULL)
#define PTE_ADDR_MASK (0x000FFFFFFFFFF000ULL)

namespace arch::x86_64::vmm
{
	void init();

	void set_cr3(uintptr_t new_cr3);
	uintptr_t get_cr3();

	void* alloc_pages(uint64_t npages);
	void free_pages(void* ptr, uint64_t npages);
	void* malloc(uint64_t nbytes);
	void free(void* ptr, uint64_t nbytes);
	uint64_t free_pages_count();
	uint64_t total_pages_count();

	void* mmap(void* phys, void* virt, uint64_t flag, uint64_t npages);
	void munmap(void* virt, uint64_t npages);
}
