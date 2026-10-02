#pragma once

#include <cstdint>

struct [[gnu::packed]] acpi_sdt_header
{
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_rev;
	uint32_t creator_id;
	uint32_t creator_rev;
};

// cast acpi_table* to the wanted table's struct pointer
struct [[gnu::packed]] acpi_table
{
	acpi_sdt_header header;
};

namespace arch::x86_64::acpi
{
	void init();

	acpi_table* get_table(const char* sig);
}
