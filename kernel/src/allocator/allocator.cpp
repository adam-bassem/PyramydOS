#include "allocator.hpp"

#include <utils.hpp>
#include <cstdint>
#include <cstddef>

#if defined (__x86_64__)
#	include <arch/x86_64/mm/vmm.hpp>
#	include <arch/x86_64/mm/pmm.hpp>
#endif

namespace
{
	constexpr uint32_t alloc_header_marker = 0xDEADBEEF;
	constexpr uint32_t free_header_marker = 0xCAFEBABE;
	constexpr size_t page_size = 0x1000;
	constexpr size_t region_grow_pages = 16;

	struct [[gnu::packed]] alloc_hdr
	{
		uint32_t marker;
		size_t size;
	};

	struct free_hdr
	{
		uint32_t marker;
		size_t size;
		free_hdr* prev;
		free_hdr* next;
	};

	free_hdr* free_head = nullptr;
	uint64_t heap_cursor = 0;

	inline size_t align_up(size_t n, size_t a)
	{
		return (n + a - 1) & ~(a - 1);
	}

	void free_list_insert(free_hdr* f)
	{
		f->prev = nullptr;
		f->next = free_head;
		if (free_head) free_head->prev = f;
		free_head = f;
	}

	void free_list_remove(free_hdr* f)
	{
		if (f->prev) f->prev->next = f->next;
		else free_head = f->next;

		if (f->next) f->next->prev = f->prev;

		f->prev = nullptr;
		f->next = nullptr;
	}

	free_hdr* find_fit(size_t need)
	{
		free_hdr* f = free_head;
		while (f && f->size + sizeof(free_hdr) < need) f = f->next;
		return f;
	}

	void* arch_map_pages(uint64_t virt, size_t pages)
	{
#if defined (__x86_64__)
		void* phys = arch::x86_64::pmm::alloc_pages(pages);
		if (!phys) return nullptr;

		if (!arch::x86_64::vmm::mmap(reinterpret_cast<void*>(virt), phys, PTE_PRESENT | PTE_WRITABLE, pages * page_size))
			return nullptr;

		return reinterpret_cast<void*>(virt);
#else
		return nullptr;
#endif
	}

	bool grow_heap(size_t min_bytes)
	{
		size_t pages = align_up(min_bytes, page_size) / page_size;
		if (pages < region_grow_pages) pages = region_grow_pages;

		void* virt = arch_map_pages(heap_cursor, pages);
		if (!virt) return false;

		free_hdr* f = reinterpret_cast<free_hdr*>(virt);
		f->marker = free_header_marker;
		f->size = pages * page_size - sizeof(free_hdr);

		free_list_insert(f);
		heap_cursor += pages * page_size;

		return true;
	}

	void* carve(free_hdr* f, size_t size)
	{
		size_t total = f->size + sizeof(free_hdr);
		free_list_remove(f);

		size_t alloc_size = total;

		if (total - size >= sizeof(free_hdr) + 16)
		{
			free_hdr* remainder = reinterpret_cast<free_hdr*>(reinterpret_cast<uint8_t*>(f) + size);
			remainder->marker = free_header_marker;
			remainder->size = total - size - sizeof(free_hdr);
			free_list_insert(remainder);

			alloc_size = size;
		}

		alloc_hdr* hdr = reinterpret_cast<alloc_hdr*>(f);
		hdr->marker = alloc_header_marker;
		hdr->size = alloc_size - sizeof(alloc_hdr);

		return reinterpret_cast<uint8_t*>(hdr) + sizeof(alloc_hdr);
	}
}

void alloc::init(void* base)
{
	heap_cursor = (uint64_t)base;
	grow_heap(region_grow_pages * page_size);
}

void* alloc::malloc(size_t size)
{
	if (!size) return nullptr;

	size_t need = align_up(size + sizeof(alloc_hdr), 16);

	free_hdr* f = find_fit(need);
	if (!f)
	{
		if (!grow_heap(need)) return nullptr;
		f = find_fit(need);
		if (!f) return nullptr;
	}

	return carve(f, need);
}

void* alloc::calloc(size_t nmemb, size_t size)
{
	if (!nmemb || !size) return nullptr;
	if (size > (~size_t(0)) / nmemb) return nullptr;

	size_t total = nmemb * size;
	void* ptr = malloc(total);
	if (!ptr) return nullptr;

	memset(ptr, 0, total);
	return ptr;
}

void* alloc::realloc(void* ptr, size_t size)
{
	if (!ptr) return malloc(size);

	if (!size)
	{
		free(ptr);
		return nullptr;
	}

	alloc_hdr* hdr = reinterpret_cast<alloc_hdr*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(alloc_hdr));

	void* new_ptr = malloc(size);
	if (!new_ptr) return nullptr;

	size_t copy_size = hdr->size < size ? hdr->size : size;
	memcpy(new_ptr, ptr, copy_size);

	free(ptr);
	return new_ptr;
}

void alloc::free(void* ptr)
{
	if (!ptr) return;

	alloc_hdr* hdr = reinterpret_cast<alloc_hdr*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(alloc_hdr));
	if (hdr->marker != alloc_header_marker) return;

	free_hdr* f = reinterpret_cast<free_hdr*>(hdr);
	f->marker = free_header_marker;
	f->size = hdr->size + sizeof(alloc_hdr) - sizeof(free_hdr);

	free_list_insert(f);
}
