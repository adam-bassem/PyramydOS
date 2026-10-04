#include "vmm.hpp"
#include "pmm.hpp"

#include <utils.hpp>

namespace
{
	uintptr_t boot_cr3;

	void* translate_phys_to_virt(uintptr_t ptr)
	{
		if (ptr >= hhdm_request.response->offset) return (void*)ptr;
		else return (void*)(ptr + hhdm_request.response->offset);
	}

	uint64_t* split_huge(uint64_t* table, uint64_t index, uint64_t entry, uint64_t shift)
	{
		void* phys = arch::x86_64::pmm::alloc_pages(1);
		if (!phys) return nullptr;

		uint64_t* virt = (uint64_t*)translate_phys_to_virt((uintptr_t)phys);

		uint64_t step = 1ull << (shift - 9);
		uint64_t base = entry & (shift == 30 ? HUGE_1G_MASK : HUGE_2M_MASK);
		uint64_t flags = entry & (0xFFFull | PTE_NX);
		if (shift == 21) flags &= ~PTE_HUGE;

		for (uint64_t i = 0; i < 512; i++)
			virt[i] = (base + i * step) | flags;

		table[index] = (uintptr_t)phys | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
		return virt;
	}

	uint64_t* get_table(uint64_t* table, uint64_t index, uint64_t shift, bool create)
	{
		uint64_t entry = table[index];

		if (entry & PTE_PRESENT)
		{
			if (shift && (entry & PTE_HUGE))
			{
				if (!create) return nullptr;
				return split_huge(table, index, entry, shift);
			}

			if (create && (entry & (PTE_WRITABLE | PTE_USER)) != (PTE_WRITABLE | PTE_USER))
				table[index] = entry | PTE_WRITABLE | PTE_USER;

			return (uint64_t*)translate_phys_to_virt(entry & PTE_ADDR_MASK);
		}

		if (!create) return nullptr;

		void* phys = arch::x86_64::pmm::alloc_pages(1);
		if (!phys) return nullptr;

		uint64_t* virt = (uint64_t*)translate_phys_to_virt((uintptr_t)phys);
		memset(virt, 0, 0x1000);

		table[index] = (uintptr_t)phys | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
		return virt;
	}

	uint64_t* get_pt(uintptr_t va, bool create)
	{
		uint64_t* pml4 = (uint64_t*)translate_phys_to_virt(arch::x86_64::vmm::get_cr3() & PTE_ADDR_MASK);

		uint64_t* pdpt = get_table(pml4, (va >> 39) & 0x1ff, 0, create);
		if (!pdpt) return nullptr;
		uint64_t* pd = get_table(pdpt, (va >> 30) & 0x1ff, 30, create);
		if (!pd) return nullptr;
		return get_table(pd, (va >> 21) & 0x1ff, 21, create);
	}

	uintptr_t translate_virt_to_phys(uintptr_t va)
	{
		uint64_t* pml4 = (uint64_t*)translate_phys_to_virt(arch::x86_64::vmm::get_cr3() & PTE_ADDR_MASK);

		uint64_t entry = pml4[(va >> 39) & 0x1ff];
		if (!(entry & PTE_PRESENT)) return 0;

		uint64_t* pdpt = (uint64_t*)translate_phys_to_virt(entry & PTE_ADDR_MASK);
		entry = pdpt[(va >> 30) & 0x1ff];
		if (!(entry & PTE_PRESENT)) return 0;
		if (entry & PTE_HUGE) return (entry & HUGE_1G_MASK) | (va & 0x3FFFFFFFull);

		uint64_t* pd = (uint64_t*)translate_phys_to_virt(entry & PTE_ADDR_MASK);
		entry = pd[(va >> 21) & 0x1ff];
		if (!(entry & PTE_PRESENT)) return 0;
		if (entry & PTE_HUGE) return (entry & HUGE_2M_MASK) | (va & 0x1FFFFFull);

		uint64_t* pt = (uint64_t*)translate_phys_to_virt(entry & PTE_ADDR_MASK);
		entry = pt[(va >> 12) & 0x1ff];
		if (!(entry & PTE_PRESENT)) return 0;

		return (entry & PTE_ADDR_MASK) | (va & 0xFFF);
	}

	void invlpg(void* addr)
	{
		asm volatile ("invlpg (%0)" : : "r" (addr) : "memory");
	}

	bool table_empty(uint64_t* table)
	{
		for (uint64_t i = 0; i < 512; i++)
			if (table[i]) return false;
		return true;
	}

	void release_table(uint64_t* parent, uint64_t index)
	{
		uint64_t entry = parent[index];
		parent[index] = 0;
		arch::x86_64::pmm::free_pages((void*)(entry & PTE_ADDR_MASK), 1);
	}

	void unmap_page(uintptr_t va)
	{
		uint64_t* pml4 = (uint64_t*)translate_phys_to_virt(arch::x86_64::vmm::get_cr3() & PTE_ADDR_MASK);

		uint64_t pml4_idx = (va >> 39) & 0x1ff;
		uint64_t pdpt_idx = (va >> 30) & 0x1ff;
		uint64_t pd_idx = (va >> 21) & 0x1ff;
		uint64_t pt_idx = (va >> 12) & 0x1ff;

		uint64_t* pdpt = get_table(pml4, pml4_idx, 0, false);
		if (!pdpt) return;
		uint64_t* pd = get_table(pdpt, pdpt_idx, 30, false);
		if (!pd) return;
		uint64_t* pt = get_table(pd, pd_idx, 21, false);
		if (!pt) return;

		pt[pt_idx] = 0;
		invlpg((void*)va);

		if (!table_empty(pt)) return;
		release_table(pd, pd_idx);

		if (!table_empty(pd)) return;
		release_table(pdpt, pdpt_idx);

		if (pml4_idx >= 256 || !table_empty(pdpt)) return;
		release_table(pml4, pml4_idx);
	}
}

void arch::x86_64::vmm::init()
{
	asm volatile ("mov %%cr3, %0" : "=r" (boot_cr3) : : "memory");
}

void arch::x86_64::vmm::set_cr3(uintptr_t new_cr3)
{
	if (!new_cr3) new_cr3 = boot_cr3;
	asm volatile ("mov %0, %%cr3" : : "r" (new_cr3) : "memory");
}

uintptr_t arch::x86_64::vmm::get_cr3()
{
	uintptr_t current_cr3;
	asm volatile ("mov %%cr3, %0" : "=r" (current_cr3) : : "memory");
	return current_cr3;
}

void* arch::x86_64::vmm::alloc_pages(uint64_t npages)
{
	void* phys = arch::x86_64::pmm::alloc_pages(npages);
	if (!phys) return nullptr;
	return translate_phys_to_virt((uintptr_t)phys);
}

void arch::x86_64::vmm::free_pages(void* ptr, uint64_t npages)
{
	uintptr_t phys = translate_virt_to_phys((uintptr_t)ptr);
	if (!phys) return;
	arch::x86_64::pmm::free_pages((void*)phys, npages);
}

void* arch::x86_64::vmm::malloc(uint64_t nbytes)
{
	void* phys = arch::x86_64::pmm::malloc(nbytes);
	if (!phys) return nullptr;
	return translate_phys_to_virt((uintptr_t)phys);
}

void arch::x86_64::vmm::free(void* ptr, uint64_t nbytes)
{
	uintptr_t phys = translate_virt_to_phys((uintptr_t)ptr);
	if (!phys) return;
	arch::x86_64::pmm::free((void*)phys, nbytes);
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
	for (uint64_t i = 0; i < npages; i++)
	{
		uintptr_t va = (uintptr_t)virt + i * 0x1000;
		uintptr_t pa = (uintptr_t)phys + i * 0x1000;

		uint64_t* pt = get_pt(va, true);
		if (!pt) return nullptr;

		pt[(va >> 12) & 0x1ff] = (pa & PTE_ADDR_MASK) | flags | PTE_PRESENT;
		invlpg((void*)va);
	}

	return virt;
}

void arch::x86_64::vmm::munmap(void* virt, uint64_t npages)
{
	for (uint64_t i = 0; i < npages; i++)
		unmap_page((uintptr_t)virt + i * 0x1000);
}
