#include "utils.hpp"

namespace
{

__attribute__((used, section(".limine_requests")))
volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

}

namespace
{

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
	.response = nullptr
};

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

void hcf() {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}

extern "C" {
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
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

	for (size_t i = 0; &__init_array[i] != __init_array_end; i++) {
        __init_array[i]();
    }

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

// ring of death kinda style stuff
void hcf_g()
{
    auto* fb = framebuffer_request.response->framebuffers[0];
    auto* pixels = reinterpret_cast<uint32_t*>(fb->address);

    size_t stride = fb->pitch / 4;

    constexpr size_t BORDER = 4;
    constexpr uint32_t RED = 0xFFFF0000;

    auto putpx = [pixels, stride](size_t x, size_t y,
                                  uint32_t colour)
    {
        pixels[y * stride + x] = colour;
    };

    for (size_t y = 0; y < BORDER; y++)
    {
        for (size_t x = 0; x < fb->width; x++)
        {
            putpx(x, y, RED);
            putpx(x, fb->height - BORDER + y, RED);
        }
    }

    for (size_t y = BORDER; y < fb->height - BORDER; y++)
    {
        for (size_t x = 0; x < BORDER; x++)
        {
            putpx(x, y, RED);
            putpx(fb->width - BORDER + x, y, RED);
        }
    }

    hcf();
}
