#pragma once

#include <stdint.h>

#define REG_ID                  0x020
#define REG_VERSION             0x030
#define REG_TPR                 0x080
#define REG_APR                 0x090
#define REG_PPR                 0x0A0
#define REG_EOI                 0x0B0
#define REG_RRD                 0x0C0
#define REG_LOGICAL_DEST        0x0D0
#define REG_DEST_FORMAT         0x0E0
#define REG_SPURIOUS            0x0F0

#define REG_ISR                 0x100
#define REG_TMR                 0x180
#define REG_IRR                 0x200

#define REG_ERROR_STATUS        0x280

#define REG_ICR_LOW             0x300
#define REG_ICR_HIGH            0x310

#define REG_LVT_TIMER           0x320
#define REG_LVT_THERMAL         0x330
#define REG_LVT_PERFORMANCE     0x340
#define REG_LVT_LINT0           0x350
#define REG_LVT_LINT1           0x360
#define REG_LVT_ERROR           0x370

#define REG_TIMER_INITIAL_COUNT 0x380
#define REG_TIMER_CURRENT_COUNT 0x390
#define REG_TIMER_DIVIDE        0x3E0

#define MSR_BASE 0x800

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
