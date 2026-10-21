#pragma once

#include <stdint.h>

constexpr uint32_t reg_id = 0x020;
constexpr uint32_t reg_version = 0x030;
constexpr uint32_t reg_tpr = 0x080;
constexpr uint32_t reg_apr = 0x090;
constexpr uint32_t reg_ppr = 0x0A0;
constexpr uint32_t reg_eoi = 0x0B0;
constexpr uint32_t reg_rrd = 0x0C0;
constexpr uint32_t reg_logical_dest = 0x0D0;
constexpr uint32_t reg_dest_format = 0x0E0;
constexpr uint32_t reg_spurious = 0x0F0;

constexpr uint32_t reg_isr = 0x100;
constexpr uint32_t reg_tmr = 0x180;
constexpr uint32_t reg_irr = 0x200;

constexpr uint32_t reg_error_status = 0x280;

constexpr uint32_t reg_icr_low = 0x300;
constexpr uint32_t reg_icr_high = 0x310;

constexpr uint32_t reg_lvt_timer = 0x320;
constexpr uint32_t reg_lvt_thermal = 0x330;
constexpr uint32_t reg_lvt_performance = 0x340;
constexpr uint32_t reg_lvt_lint0 = 0x350;
constexpr uint32_t reg_lvt_lint1 = 0x360;
constexpr uint32_t reg_lvt_error = 0x370;

constexpr uint32_t reg_timer_initial_count = 0x380;
constexpr uint32_t reg_timer_current_count = 0x390;
constexpr uint32_t reg_timer_divide = 0x3E0;

namespace arch::x86_64::lapic
{
	class lapic
	{
	public:
		void init(void* vbase, bool use_x2apic);
		uint32_t read(uint32_t reg) const;
		void write(uint32_t reg, uint32_t value);
		void send_eoi();
		void enable();

	private:
		void* base = nullptr;
		bool x2apic = false;
	};

	extern lapic lapic_bsp;
}
