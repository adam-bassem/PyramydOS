#pragma once

#include <cstdint>

namespace arch::x86_64::ioapic
{
    void init();
    void add_ioapic(void* address, uint32_t gsi_base);
    void register_iso(uint8_t bus_src, uint8_t irq_src, uint32_t gsi);
    uint32_t get_gsi_for_irq(uint32_t irq);
    void set_redir_entry(uint32_t gsi, uint8_t vector, bool mask);
}
