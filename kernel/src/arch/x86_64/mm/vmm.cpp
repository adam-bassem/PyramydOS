#include "vmm.hpp"
#include "pmm.hpp"

#include <utils.hpp>

namespace
{
	uintptr_t cr3;

	void* translate_phys_to_virt(uintptr_t ptr)
	{
		if (ptr >= hhdm_request.response->offset) return (void*)ptr;
		else return (void*)(ptr + hhdm_request.response->offset);
	}

	void* translate_virt_to_phys(uintptr_t ptr)
	{
		if (ptr <= 0x00007FFFFFFFFFFF) return (void*)ptr;
		else return (void*)(ptr - hhdm_request.response->offset);
	}

	uint64_t* get_or_create_table(uint64_t* table, uint64_t index)
	{
		uint64_t entry = table[index];

		if (entry & PTE_PRESENT)
			return (uint64_t*)translate_phys_to_virt(entry & PTE_ADDR_MASK);

		void* phys = arch::x86_64::pmm::alloc_pages(1);
		uint64_t* virt = (uint64_t*)translate_phys_to_virt((uintptr_t)phys);
		memset(virt, 0, 0x1000);

		table[index] = (uintptr_t)phys | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
		return virt;
	}

	uint64_t* get_table(uint64_t* table, uint64_t index)
	{
		uint64_t entry = table[index];

		if (!(entry & PTE_PRESENT)) return nullptr;

		return (uint64_t*)translate_phys_to_virt(entry & PTE_ADDR_MASK);
	}

	void invlpg(void* addr)
	{
		asm volatile ("invlpg (%0)" : : "r" (addr) : "memory");
	}
}

void arch::x86_64::vmm::init()
{
	asm volatile ("mov %%cr3, %0" : "=r" (cr3) : : "memory");
}

void arch::x86_64::vmm::set_cr3(uintptr_t new_cr3)
{
	if (!new_cr3) new_cr3 = cr3;
	asm volatile ("mov %0, %%cr3" : : "r" (new_cr3) : "memory");
}

uintptr_t arch::x86_64::vmm::get_cr3()
{
	uintptr_t __cr3__;
	asm volatile ("mov %%cr3, %0" : "=r" (__cr3__) : : "memory");
	return __cr3__;
}

void* arch::x86_64::vmm::alloc_pages(uint64_t npages)
{
	return translate_phys_to_virt((uintptr_t)arch::x86_64::pmm::alloc_pages(npages));
}

void arch::x86_64::vmm::free_pages(void* ptr, uint64_t npages)
{
	arch::x86_64::pmm::free_pages(translate_virt_to_phys((uintptr_t)ptr), npages);
}

void* arch::x86_64::vmm::malloc(uint64_t nbytes)
{
	return translate_phys_to_virt((uintptr_t)arch::x86_64::pmm::malloc(nbytes));
}

void arch::x86_64::vmm::free(void* ptr, uint64_t nbytes)
{
	arch::x86_64::pmm::free(translate_virt_to_phys((uintptr_t)ptr), nbytes);
}

uint64_t arch::x86_64::vmm::free_pages_count()
{
	return arch::x86_64::pmm::free_pages_count();
}

uint64_t arch::x86_64::vmm::total_pages_count()
{
	return arch::x86_64::pmm::total_pages_count();
}

void* arch::x86_64::vmm::mmap(void* phys, void* virt, uint64_t flags, uint64_t npages)
{
	uint64_t* pml4 = (uint64_t*)translate_phys_to_virt(cr3 & PTE_ADDR_MASK);

	for (uint64_t i = 0; i < npages; i++)
	{
		uintptr_t va = (uintptr_t)virt + i * 0x1000;
		uintptr_t pa = (uintptr_t)phys + i * 0x1000;

		uint64_t pml4_idx = (va >> 39) & 0x1ff;
		uint64_t pdpt_idx = (va >> 30) & 0x1ff;
		uint64_t pd_idx = (va >> 21) & 0x1ff;
		uint64_t pt_idx = (va >> 12) & 0x1ff;

		uint64_t* pdpt = get_or_create_table(pml4, pml4_idx);
		uint64_t* pd = get_or_create_table(pdpt, pdpt_idx);
		uint64_t* pt = get_or_create_table(pd, pd_idx);

		pt[pt_idx] = (pa & PTE_ADDR_MASK) | flags | PTE_PRESENT;
		invlpg((void*)va);
	}

	return virt;
}

void arch::x86_64::vmm::munmap(void* virt)
{
	uint64_t* pml4 = (uint64_t*)translate_phys_to_virt(cr3 & PTE_ADDR_MASK);
	uintptr_t va = (uintptr_t)virt;

	uint64_t pml4_idx = (va >> 39) & 0x1ff;
	uint64_t pdpt_idx = (va >> 30) & 0x1ff;
	uint64_t pd_idx = (va >> 21) & 0x1ff;
	uint64_t pt_idx = (va >> 12) & 0x1ff;

	uint64_t* pdpt = get_table(pml4, pml4_idx);
	if (!pdpt) return;
	uint64_t* pd = get_table(pdpt, pdpt_idx);
	if (!pd) return;
	uint64_t* pt = get_table(pd, pd_idx);
	if (!pt) return;

	pt[pt_idx] = 0;
	invlpg((void*)va);
}
