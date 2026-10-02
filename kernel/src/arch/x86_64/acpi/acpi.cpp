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
		console::kprintf("\e[1;31mRSDP address is \e[1;35mNULL\r\n\e[0mHalting...\r\n");
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
		console::kprintf("\e[1;31mRSDP checksum is INVALID\r\n\e[0mHalting...\r\n");
		hcf();
	}
	console::kprintf("RSDP checksum valid\r\n");

	console::kprintf("Found RSDP table...\r\n");
	console::kprintf("Table signature: \e[1;36m\"%.8s\"\e[0m\r\n", rsdp->sig);
	console::kprintf("Checksum: %u\r\n", rsdp->checksum);
	console::kprintf("OEM ID: \e[1;36m\"%.6s\"\e[0m\r\n", rsdp->oem_id);
	console::kprintf("Revision: %u\r\n", rsdp->revision);
	console::kprintf("RSDT address: 0x%x\r\n", rsdp->rsdt_addr);

	if (rsdp->revision < 2)
	{
		console::kprintf("revision < 2, unsupported version, halting...\r\n");
		hcf();
	}

	console::kprintf("RSDP version 2.0+\r\n");
	console::kprintf("XSDT address: 0x%llx\r\n", rsdp->xsdt_addr);
	console::kprintf("Extended checksum: %u\r\n", rsdp->extended_checksum);

	sum = 0;
	for (uint32_t i = 0; i < rsdp->length; i++)
	{
		sum += reinterpret_cast<uint8_t*>(rsdp)[i];
	}
	if (sum != 0)
	{
		console::kprintf("\e[1;31mRSDP 2.0 checksum is INVALID\r\n\e[0mHalting...\r\n");
		hcf();
	}
	console::kprintf("RSDP 2.0 checksum valid\r\n");

	if (!rsdp->xsdt_addr)
	{
		console::kprintf("\e[1;31mXSDT address is \e[1;35mNULL\r\n\e[0mHalting...\r\n");
		hcf();
	}

	xsdt = reinterpret_cast<acpi_sdt_header*>(phys_to_virt(rsdp->xsdt_addr));

	if (!check_checksum(xsdt))
	{
		console::kprintf("\e[1;31mXSDT checksum is INVALID\r\n\e[0mHalting...\r\n");
		hcf();
	}
	console::kprintf("XSDT checksum valid\r\n");

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
			console::kprintf("\e[1;31mACPI table %.4s checksum is INVALID\r\n\e[0m", table->header.signature);
			continue;
		}

		acpi_table_entry* entry = reinterpret_cast<acpi_table_entry*>(alloc::malloc(sizeof(acpi_table_entry)));
		if (!entry)
		{
			console::kprintf("\e[1;31mOut of memory while enumerating ACPI tables\r\n\e[0m");
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

		console::kprintf("Discovered table: \e[1;36m%.4s\t\e[1;35m%.6s\t\e[1;32m0x%p\r\n\e[0m", table->header.signature, table->header.oem_id, table);
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
