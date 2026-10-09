#include "utils.hpp"

#if defined(__x86_64__)
#	include <arch/x86_64/cpuid.hpp>
#endif
#include <timers/timers.hpp>
#include <console/console.hpp>

namespace
{
	__attribute__((used, section(".limine_requests")))
	volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

	__attribute__((used, section(".limine_requests_start")))
	volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

	__attribute__((used, section(".limine_requests_end")))
	volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

}

__attribute__((used, section(".limine_requests")))
volatile struct limine_memmap_request memmap_request =
{
	.id = LIMINE_MEMMAP_REQUEST_ID,
	.revision = 0,
	.response = nullptr
};

__attribute__((used, section(".limine_requests")))
volatile struct limine_hhdm_request hhdm_request =
{
	.id = LIMINE_HHDM_REQUEST_ID,
	.revision = 0,
	.response = nullptr
};

__attribute__((used, section(".limine_requests")))
volatile struct limine_framebuffer_request framebuffer_request =
{
	.id = LIMINE_FRAMEBUFFER_REQUEST_ID,
	.revision = 0,
	.response = nullptr
};

#if defined(__x86_64__)
__attribute__((used, section(".limine_requests")))
volatile struct limine_rsdp_request rsdp_request =
{
	.id = LIMINE_RSDP_REQUEST_ID,
	.revision = 0,
	.response = nullptr
};

__attribute__((used, section(".limine_requests")))
volatile struct limine_mp_request mp_request =
{
	.id = LIMINE_MP_REQUEST_ID,
	.revision = 0,
	.response = nullptr,
	.flags = 0
};

namespace
{
	bool has_rdrand = false;
	bool has_rdseed = false;
#endif

	uint64_t state = 0;

	uint64_t time_rand()
	{
		if (state == 0)
		{
			state =
				timers::total_elapsed() * 1000000 +
				timers::total_ms_elapsed() * 1000 +
				timers::total_us_elapsed();
		}

		state += 0x9E3779B97F4A7C15;

		uint64_t z = state;
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
		z ^= z >> 31;
		return z;
	}
}

#if defined(__x86_64__)
uintptr_t phys_to_virt(uintptr_t phys)
{
	return phys <= 0x00007FFFFFFFFFFF ? phys + 0xFFFF800000000000 : phys;
}

void* phys_to_virt(void* phys)
{
	return (void*)phys_to_virt((uintptr_t)phys);
}

uintptr_t virt_to_phys(uintptr_t virt)
{
	return virt >= 0xFFFF800000000000 ? virt : virt - 0xFFFF800000000000;
}

void* virt_to_phys(void* virt)
{
	return (void*)virt_to_phys((uintptr_t)virt);
}
#endif

void hcf()
{
	for (;;)
	{
#if defined(__x86_64__)
		asm ("hlt");
#elif defined(__aarch64__) || defined(__riscv)
		asm ("wfi");
#elif defined(__loongarch64)
		asm ("idle 0");
#endif
	}
}

extern "C"
{
	int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
	void __cxa_pure_virtual() { hcf(); }
	void __cxa_deleted_virtual() { hcf(); }
	void *__dso_handle;
	int __cxa_guard_acquire(uint64_t *guard) { return *reinterpret_cast<uint8_t *>(guard) == 0; }
	void __cxa_guard_release(uint64_t *guard) { *reinterpret_cast<uint8_t *>(guard) = 1; }
}

extern void (*__init_array[])();
extern void (*__init_array_end[])();

void pre_kernel()
{
	if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false)
		hcf();

	for (size_t i = 0; &__init_array[i] != __init_array_end; i++)
		__init_array[i]();

	if (!memmap_request.response ||
		memmap_request.response->entry_count < 1 ||
		!memmap_request.response->entries ||
		!hhdm_request.response ||
		!framebuffer_request.response ||
		framebuffer_request.response->framebuffer_count < 1 ||
		!framebuffer_request.response->framebuffers)
	{
		hcf();
	}
}

void init_rand()
{
#if defined(__x86_64__)
	cpuid_result r = arch::x86_64::cpuid::cpuid(0, 0);
	uint32_t max_leaf = r.eax;

	if (max_leaf >= 1)
	{
		r = arch::x86_64::cpuid::cpuid(1, 0);

		has_rdrand = (r.ecx & (1u << 30)) != 0;

		if (has_rdrand)
			console::kprintf("RDRAND supported");
	}

	if (max_leaf >= 7)
	{
		r = arch::x86_64::cpuid::cpuid(7, 0);

		has_rdseed = (r.ebx & (1u << 18)) != 0;

		if (has_rdseed)
			console::kprintf("RDSEED supported");
	}
#endif
}

#if defined(__x86_64__)
bool rdseed64(uint64_t* out)
{
	uint8_t success;
	asm volatile("rdseed %0; setc %1" : "=r"(*out), "=qm"(success) : : "cc");
	return success;
}

bool rdrand64(uint64_t* out)
{
	uint8_t success;
	asm volatile("rdrand %0; setc %1" : "=r"(*out), "=qm"(success) : : "cc");
	return success;
}
#endif

uint64_t rand64()
{
#if defined(__x86_64__)
	uint64_t value;

	if (has_rdseed)
	{
		for (int i = 0; i < 10; ++i)
		{
			if (rdseed64(&value))
				return value;
		}
	}

	if (has_rdrand)
	{
		for (int i = 0; i < 10; ++i)
		{
			if (rdrand64(&value))
				return value;
		}
	}
#endif

	return time_rand();
}

rmethod rand_method()
{
	if (has_rdrand)
		return RAND_METHOD_RDRAND;
	return RAND_METHOD_SPLITMIX64;
}

UUID uuid_gen()
{
	UUID uuid;

	uint64_t a = rand64();
	uint64_t b = rand64();

	uuid.low = a;
	uuid.high = b;

	uuid.bytes[6] = (uuid.bytes[6] & 0x0F) | 0x40;
	uuid.bytes[8] = (uuid.bytes[8] & 0x3F) | 0x80;

	return uuid;
}

void print_uuid(const UUID& uuid)
{
    printf(
        "%02x%02x%02x%02x-"
        "%02x%02x-"
        "%02x%02x-"
        "%02x%02x-"
        "%02x%02x%02x%02x%02x%02x",
        uuid.bytes[0],
        uuid.bytes[1],
        uuid.bytes[2],
        uuid.bytes[3],
        uuid.bytes[4],
        uuid.bytes[5],
        uuid.bytes[6],
        uuid.bytes[7],
        uuid.bytes[8],
        uuid.bytes[9],
        uuid.bytes[10],
        uuid.bytes[11],
        uuid.bytes[12],
        uuid.bytes[13],
        uuid.bytes[14],
        uuid.bytes[15]
    );
}

int strlen(const char* __restrict s)
{
	if (!s)
		return 0;

	int n = 0;
	while (*s)
	{
		n++;
		s++;
	}
	return n;
}

int strcmp(const char* s1, const char* s2)
{
	while (*s1 && (*s1 == *s2))
	{
		s1++;
		s2++;
	}
	return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strtoks(char* str, const char* delms, char** save)
{
	if (!str)
	{
		str = *save;
	}
	
	if (!str)
	{
		return nullptr;
	}

	while (*str)
	{
		bool delm = false;

		for (const char* d = delms; *d; d++)
		{
			if (*str == *d)
			{
				delm = true;
				break;
			}
		}

		if (!delm)
		{
			break;
		}

		str++;
	}

	if (!*str)
	{
		*save = nullptr;
		return nullptr;
	}

	char* token = str;

	while (*str)
	{
		for (const char* d = delms; *d; d++)
		{
			if (*str == *d)
			{
				*str = 0;
				*save = str + 1;
				return token;
			}
		}

		str++;
	}

	*save = nullptr;
	return token;
}
