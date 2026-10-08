#pragma once

#include <limine.h>
#include <cstdint>
#include <cstddef>

// =================

#if defined(__x86_64__)
uintptr_t phys_to_virt(uintptr_t phys);
void* phys_to_virt(void* phys);

uintptr_t virt_to_phys(uintptr_t virt);
void* virt_to_phys(void* virt);
#endif

// =================

void hcf();
void pre_kernel();

// =================

extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_framebuffer_request framebuffer_request;

#if defined(__x86_64__)
extern volatile struct limine_rsdp_request rsdp_request;
extern volatile struct limine_mp_request mp_request;
#endif

// =================

typedef struct __int_context
{
	uint64_t cr4;
	uint64_t cr3;
	uint64_t cr2;
	uint64_t cr0;

	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;

	uint64_t rsi;
	uint64_t rdi;
	uint64_t rbp;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;

	uint64_t vector;
	uint64_t error_code;

	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;

} int_context_t;

// =================

extern "C" {

	void *memcpy(void *__restrict dest, const void *__restrict src, size_t n);
	void *memset(void *s, int c, size_t n);
	void *memmove(void *dest, const void *src, size_t n);
	int memcmp(const void *s1, const void *s2, size_t n);

}

// =================

enum rmethod
{
	RAND_METHOD_RDRAND,
	RAND_METHOD_SPLITMIX64
};

void init_rand();
uint64_t rand64();
rmethod rand_method();

// =================

union UUID
{
	uint8_t bytes[16];

	struct
	{
		uint64_t low;
		uint64_t high;
	};
};

UUID uuid_gen();
void print_uuid(const UUID& uuid);

// =================

int strlen(const char* __restrict s);
