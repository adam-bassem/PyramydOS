#include "pmm.hpp"
#include <utils.hpp>
#include <cstddef>
#include <cstdint>

namespace
{
	constexpr uint64_t page_size = 0x1000;
	constexpr uint64_t page_mask = page_size - 1;
	constexpr uint64_t bootstrap_pool_pages = 16;
	constexpr uint64_t descriptor_reserve = 8;

	struct memory_region
	{
		uint64_t base;
		uint64_t length_pages;
		bool free;
		memory_region* prev;
		memory_region* next;
		memory_region* free_prev;
		memory_region* free_next;
	};

	memory_region* region_head = nullptr;
	memory_region* region_tail = nullptr;
	memory_region* free_head = nullptr;
	memory_region* descriptor_head = nullptr;
	uint64_t descriptor_count = 0;
	uint64_t free_pages_total = 0;
	uint64_t total_pages = 0;
	uint64_t hhdm = 0;

	void descriptor_pool_add(uint64_t virt_base, uint64_t bytes)
	{
		uint64_t n = bytes / sizeof(memory_region);

		for (uint64_t i = 0; i < n; i++)
		{
			memory_region* d = reinterpret_cast<memory_region*>(virt_base + i * sizeof(memory_region));
			d->next = descriptor_head;
			descriptor_head = d;
			descriptor_count++;
		}
	}

	void descriptor_release(memory_region* d)
	{
		d->next = descriptor_head;
		descriptor_head = d;
		descriptor_count++;
	}

	memory_region* descriptor_acquire()
	{
		memory_region* d = descriptor_head;
		if (!d) return nullptr;

		descriptor_head = d->next;
		descriptor_count--;

		d->base = 0;
		d->length_pages = 0;
		d->free = false;
		d->prev = nullptr;
		d->next = nullptr;
		d->free_prev = nullptr;
		d->free_next = nullptr;

		return d;
	}

	void free_list_insert(memory_region* r)
	{
		r->free_prev = nullptr;
		r->free_next = free_head;
		if (free_head) free_head->free_prev = r;
		free_head = r;
	}

	void free_list_remove(memory_region* r)
	{
		if (r->free_prev) r->free_prev->free_next = r->free_next;
		else free_head = r->free_next;

		if (r->free_next) r->free_next->free_prev = r->free_prev;

		r->free_prev = nullptr;
		r->free_next = nullptr;
	}

	void list_insert_before(memory_region* at, memory_region* r)
	{
		r->prev = at->prev;
		r->next = at;

		if (at->prev) at->prev->next = r;
		else region_head = r;

		at->prev = r;
	}

	void list_append(memory_region* r)
	{
		r->prev = region_tail;
		r->next = nullptr;

		if (region_tail) region_tail->next = r;
		else region_head = r;

		region_tail = r;
	}

	void list_remove(memory_region* r)
	{
		if (r->prev) r->prev->next = r->next;
		else region_head = r->next;

		if (r->next) r->next->prev = r->prev;
		else region_tail = r->prev;

		r->prev = nullptr;
		r->next = nullptr;
	}

	void descriptor_refill()
	{
		if (descriptor_count >= descriptor_reserve) return;

		memory_region* r = free_head;
		if (!r) return;

		uint64_t page = r->base;
		r->base += page_size;
		r->length_pages--;
		free_pages_total--;

		if (r->length_pages == 0)
		{
			free_list_remove(r);
			list_remove(r);
			descriptor_release(r);
		}

		descriptor_pool_add(reinterpret_cast<uint64_t>(phys_to_virt(page)), page_size);
	}

	void coalesce(memory_region* r)
	{
		memory_region* n = r->next;

		if (n && n->free && r->base + r->length_pages * page_size == n->base)
		{
			r->length_pages += n->length_pages;
			free_list_remove(n);
			list_remove(n);
			descriptor_release(n);
		}

		memory_region* p = r->prev;

		if (p && p->free && p->base + p->length_pages * page_size == r->base)
		{
			p->length_pages += r->length_pages;
			free_list_remove(r);
			list_remove(r);
			descriptor_release(r);
		}
	}

	void region_append(uint64_t base, uint64_t pages)
	{
		if (!pages) return;

		memory_region* r = descriptor_acquire();
		if (!r) return;

		r->base = base;
		r->length_pages = pages;
		r->free = true;

		list_append(r);
		free_list_insert(r);

		free_pages_total += pages;
		total_pages += pages;
	}

	bool entry_span(const volatile limine_memmap_entry* entry, uint64_t& base, uint64_t& pages)
	{
		if (entry->type != LIMINE_MEMMAP_USABLE) return false;
		if (entry->base < 0x100000) return false;

		base = (entry->base + page_mask) & ~page_mask;
		uint64_t end = (entry->base + entry->length) & ~page_mask;

		if (end <= base) return false;

		pages = (end - base) / page_size;
		return true;
	}
}

void arch::x86_64::pmm::init()
{
	hhdm = hhdm_request.response->offset;

	uint64_t pool_index = 0;
	bool pool_found = false;

	for (uint64_t i = 0; i < memmap_request.response->entry_count; i++)
	{
		uint64_t base;
		uint64_t pages;

		if (!entry_span(memmap_request.response->entries[i], base, pages)) continue;
		if (pages <= bootstrap_pool_pages) continue;

		descriptor_pool_add(reinterpret_cast<uint64_t>(phys_to_virt(base)), bootstrap_pool_pages * page_size);
		pool_index = i;
		pool_found = true;
		break;
	}

	if (!pool_found) hcf_g();

	for (uint64_t i = 0; i < memmap_request.response->entry_count; i++)
	{
		uint64_t base;
		uint64_t pages;

		if (!entry_span(memmap_request.response->entries[i], base, pages)) continue;

		if (i == pool_index)
		{
			base += bootstrap_pool_pages * page_size;
			pages -= bootstrap_pool_pages;
		}

		region_append(base, pages);
	}
}

void* arch::x86_64::pmm::alloc_pages(uint64_t npages)
{
	if (!npages) return nullptr;

	descriptor_refill();

	memory_region* r = free_head;
	while (r && r->length_pages < npages) r = r->free_next;
	if (!r) return nullptr;

	if (r->length_pages == npages)
	{
		free_list_remove(r);
		r->free = false;
		free_pages_total -= npages;
		return reinterpret_cast<void*>(r->base);
	}

	memory_region* a = descriptor_acquire();
	if (!a) return nullptr;

	a->base = r->base;
	a->length_pages = npages;
	a->free = false;

	list_insert_before(r, a);

	r->base += npages * page_size;
	r->length_pages -= npages;
	free_pages_total -= npages;

	return reinterpret_cast<void*>(a->base);
}

void arch::x86_64::pmm::free_pages(void* ptr, uint64_t npages)
{
	if (!ptr) return;

	uint64_t phys = reinterpret_cast<uint64_t>(ptr);

	memory_region* r = region_head;
	while (r && r->base != phys) r = r->next;

	if (!r || r->free) return;
	if (npages && npages != r->length_pages) return;

	r->free = true;
	free_pages_total += r->length_pages;

	free_list_insert(r);
	coalesce(r);
}

void* arch::x86_64::pmm::malloc(uint64_t nbytes)
{
	if (!nbytes) return nullptr;
	return alloc_pages((nbytes + page_mask) / page_size);
}

void arch::x86_64::pmm::free(void* ptr, uint64_t nbytes)
{
	if (!ptr) return;
	free_pages(ptr, (nbytes + page_mask) / page_size);
}

uint64_t arch::x86_64::pmm::free_pages_count()
{
	return free_pages_total;
}

uint64_t arch::x86_64::pmm::total_pages_count()
{
	return total_pages;
}
