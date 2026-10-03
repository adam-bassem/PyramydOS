#include "hpet.hpp"
#include <console/console.hpp>
#include <utils.hpp>
#include <arch/x86_64/acpi/acpi.hpp>
#include <arch/x86_64/mm/vmm.hpp>

namespace
{
	struct [[gnu::packed]] address_structure
	{
		uint8_t address_space_id;
		uint8_t register_bit_width;
		uint8_t register_bit_offset;
		uint8_t reserved;
		uint64_t address;
	};
	
	struct [[gnu::packed]] hpet_table
	{
		acpi_sdt_header header;

		uint8_t hardware_rev_id;
		uint8_t comparator_count : 5;
		uint8_t counter_size : 1;
		uint8_t reserved : 1;
		uint8_t legacy_replacement : 1;
		uint16_t pci_vendor_id;
		address_structure address;
		uint8_t hpet_number;
		uint16_t minimum_tick;
		uint8_t page_protection;
	};
	
	struct hpet_timer
	{
		volatile uint64_t config;
		volatile uint64_t comparator;
		volatile uint64_t fsb_int;
		volatile uint64_t rsv;
	};
	
	struct hpet_regs
	{
		volatile uint64_t general_cap;
		volatile uint64_t rsv0;
		volatile uint64_t general_configs;
		volatile uint64_t rsv1;
		volatile uint64_t general_int;
		uint8_t rsv2[200];
		volatile uint64_t main_counter;
		volatile uint64_t rsv3;
		hpet_timer timers;
	};

	hpet_regs* hpet_reg = nullptr;

	uint32_t clk_period_fs;
	uint64_t clk_period_ns;

	bool initialised = false;
}

void arch::x86_64::timers::hpet::init()
{
	hpet_table* hpettable = reinterpret_cast<hpet_table*>(arch::x86_64::acpi::get_table("HPET"));
	if (!hpettable)
	{
		console::kprintf(ANSI_BOLD ANSI_RED "HPET table not present");
		console::kprintf("Halting...");
		hcf();
	}
	uintptr_t phys_addr = hpettable->address.address;
	
	hpet_reg = reinterpret_cast<hpet_regs*>(arch::x86_64::vmm::mmap(
		reinterpret_cast<void*>(phys_addr), 
		reinterpret_cast<void*>(phys_to_virt(phys_addr)), 
		PTE_PRESENT | PTE_WRITABLE, 
		1
	));

	if (!hpet_reg)
	{
		console::kprintf(ANSI_BOLD ANSI_RED "Failed to allocate memory for HPET");
		console::kprintf("Halting...");
		hcf();
	}

	hpet_reg->general_configs |= 0x01;

	clk_period_fs = hpet_reg->general_cap >> 32;
	clk_period_ns = clk_period_fs / 1000000ULL;
	initialised = true;
}

void arch::x86_64::timers::hpet::sleep_us(uint64_t us_time)
{
	if (!initialised) return;
	
	uint64_t target_fs = us_time * 1000000000ULL;
	uint64_t ticks_to_wait = target_fs / clk_period_fs;
	uint64_t start_tick = hpet_reg->main_counter;

	while ((hpet_reg->main_counter - start_tick) < ticks_to_wait)
	{
		asm volatile ("pause");
	}
}

void arch::x86_64::timers::hpet::sleep_ms(uint64_t ms_time)
{
	if (!initialised) return;
	sleep_us(ms_time * 1000ULL);
}

void arch::x86_64::timers::hpet::sleep(uint64_t time)
{
	if (!initialised) return;
	sleep_us(time * 1000000ULL);
}

uint64_t arch::x86_64::timers::hpet::total_timer_ticks()
{
	if (!initialised) return 0;
	return hpet_reg->main_counter;
}

uint64_t arch::x86_64::timers::hpet::total_us_elapsed()
{
	if (!initialised) return 0;
	uint64_t ticks = total_timer_ticks();
	return (ticks * clk_period_ns) / 1000ULL;
}

uint64_t arch::x86_64::timers::hpet::total_ms_elapsed()
{
	if (!initialised) return 0;
	return total_us_elapsed() / 1000ULL;
}

uint64_t arch::x86_64::timers::hpet::total_elapsed()
{
	if (!initialised) return 0;
	return total_us_elapsed() / 1000000ULL;
}
