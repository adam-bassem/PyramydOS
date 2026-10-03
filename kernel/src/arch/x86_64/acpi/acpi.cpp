#include "acpi.hpp"
#include <console/console.hpp>
#include <utils.hpp>
#include <allocator/allocator.hpp>

struct [[gnu::packed]] rsdp_table
{
	char sig[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_addr;

	uint32_t length;
	uint64_t xsdt_addr;
	uint8_t extended_checksum;
	uint8_t reserved[3];
} *rsdp;

static acpi_sdt_header* xsdt;

struct acpi_table_entry
{
	char sig[4];
	acpi_table* table;
	acpi_table_entry* next;
} root_table, *last;

static bool check_checksum(acpi_sdt_header* table)
{
	if (table->length < sizeof(acpi_sdt_header))
	{
		return false;
	}
	uint8_t sum = 0;
	uint8_t* bytes = reinterpret_cast<uint8_t*>(table);
	for (uint32_t i = 0; i < table->length; i++)
	{
		sum += bytes[i];
	}
	return sum == 0;
}

void arch::x86_64::acpi::init()
{
	if (!rsdp_request.response || !rsdp_request.response->address)
	{
		console::kprintf(ANSI_BOLD ANSI_RED "RSDP address is " ANSI_BOLD ANSI_PURPLE "NULL");
		console::kprintf("Halting...");
		hcf();
	}
	rsdp = reinterpret_cast<rsdp_table*>(rsdp_request.response->address);

	uint8_t sum = 0;
	for (int i = 0; i < 20; i++)
	{
		sum += reinterpret_cast<uint8_t*>(rsdp)[i];
	}
	if (sum != 0)
	{
		console::kprintf(ANSI_BOLD ANSI_RED "RSDP checksum is INVALID");
		console::kprintf("Halting...");
		hcf();
	}
	console::kprintf("RSDP checksum valid");

	console::kprintf("Found RSDP table...");
	console::kprintf("Table signature: " ANSI_CYAN "\"%.8s\"", rsdp->sig);
	console::kprintf("Checksum: %u", rsdp->checksum);
	console::kprintf("OEM ID: " ANSI_CYAN "\"%.6s\"", rsdp->oem_id);
	console::kprintf("Revision: %u", rsdp->revision);
	console::kprintf("RSDT address: 0x%x", rsdp->rsdt_addr);

	if (rsdp->revision < 2)
	{
		console::kprintf("Revision < 2, unsupported version");
		console::kprintf("Halting...");
		hcf();
	}

	console::kprintf("RSDP version 2.0+");
	console::kprintf("XSDT address: 0x%llx", rsdp->xsdt_addr);
	console::kprintf("Extended checksum: %u", rsdp->extended_checksum);

	sum = 0;
	for (uint32_t i = 0; i < rsdp->length; i++)
	{
		sum += reinterpret_cast<uint8_t*>(rsdp)[i];
	}
	if (sum != 0)
	{
		console::kprintf(ANSI_BOLD ANSI_RED "RSDP 2.0 checksum is INVALID");
		console::kprintf("Halting...");
		hcf();
	}
	console::kprintf("RSDP 2.0 checksum valid");

	if (!rsdp->xsdt_addr)
	{
		console::kprintf(ANSI_BOLD ANSI_RED "XSDT address is " ANSI_BOLD ANSI_PURPLE "NULL");
		console::kprintf("Halting...");
		hcf();
	}

	xsdt = reinterpret_cast<acpi_sdt_header*>(phys_to_virt(rsdp->xsdt_addr));

	if (!check_checksum(xsdt))
	{
		console::kprintf(ANSI_BOLD ANSI_RED "XSDT checksum is INVALID");
		console::kprintf("Halting...");
		hcf();
	}
	console::kprintf("XSDT checksum valid");

	root_table.sig[0] = 'R'; root_table.sig[1] = 'S';
	root_table.sig[2] = 'D'; root_table.sig[3] = 'P';
	root_table.table = reinterpret_cast<acpi_table*>(rsdp);
	root_table.next = nullptr;

	last = &root_table;

	uint32_t entry_count = (xsdt->length - sizeof(acpi_sdt_header)) / sizeof(uint64_t);
	uint8_t* tables = reinterpret_cast<uint8_t*>(xsdt) + sizeof(acpi_sdt_header);

	for (uint32_t i = 0; i < entry_count; i++)
	{
		uint64_t table_addr;
		__builtin_memcpy(&table_addr, tables + i * sizeof(uint64_t), sizeof(uint64_t));
		if (!table_addr)
		{
			continue;
		}

		acpi_table* table = reinterpret_cast<acpi_table*>(phys_to_virt(table_addr));

		if (!check_checksum(&table->header))
		{
			console::kprintf(ANSI_BOLD ANSI_RED "ACPI table %.4s checksum is INVALID", table->header.signature);
			continue;
		}

		acpi_table_entry* entry = reinterpret_cast<acpi_table_entry*>(alloc::malloc(sizeof(acpi_table_entry)));
		if (!entry)
		{
			console::kprintf(ANSI_BOLD ANSI_RED "Out of memory while enumerating ACPI tables");
			return;
		}
		entry->sig[0] = table->header.signature[0];
		entry->sig[1] = table->header.signature[1];
		entry->sig[2] = table->header.signature[2];
		entry->sig[3] = table->header.signature[3];
		entry->table = table;
		entry->next = nullptr;

		last->next = entry;
		last = entry;

		console::kprintf("Discovered table: " ANSI_CYAN "%.4s\t" ANSI_BOLD ANSI_PURPLE "%.6s\t" ANSI_BOLD ANSI_GREEN "0x%p", table->header.signature, table->header.oem_id, table);
	}
}

acpi_table* arch::x86_64::acpi::get_table(const char* s)
{
	for (acpi_table_entry* entry = &root_table; entry; entry = entry->next)
	{
		if (entry->sig[0] == s[0] && entry->sig[1] == s[1] && entry->sig[2] == s[2] && entry->sig[3] == s[3])
		{
			return entry->table;
		}
	}
	return nullptr;
}
