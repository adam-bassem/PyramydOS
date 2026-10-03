#pragma once

#include <cstdint>

namespace arch::x86_64::ioapic
{
    void init();
    void add_ioapic(void* address, uint32_t gsi_base);
    void register_iso(uint8_t bus_src, uint8_t irq_src, uint32_t gsi);
}
