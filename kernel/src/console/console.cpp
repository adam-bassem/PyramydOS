#include "console.hpp"
#include <ext/flanterm/flanterm.h>
#include <ext/flanterm/flanterm_backends/fb.h>
#include <utils.hpp>
#include <allocator/allocator.hpp>
#include <timers/timers.hpp>

#include <cstdint>

namespace
{
	constexpr size_t ctx_count = 4;

	bool console_initialised = false;
	struct limine_framebuffer* fb;
	struct flanterm_context* ft_ctxs[ctx_count];
	size_t active_ctx = 0;

	void flanterm_free(void* ptr, size_t)
	{
		alloc::free(ptr);
	}

	struct flanterm_context* make_ctx()
	{
		return flanterm_fb_init(
			alloc::malloc,
			flanterm_free,
			reinterpret_cast<uint32_t*>(fb->address), fb->width, fb->height, fb->pitch,
			fb->red_mask_size, fb->red_mask_shift,
			fb->green_mask_size, fb->green_mask_shift,
			fb->blue_mask_size, fb->blue_mask_shift,
			nullptr,
			nullptr, nullptr,
			nullptr, nullptr,
			nullptr, nullptr,
			nullptr, 0, 0, 1,
			0, 0,
			0,
			0,
			true
		);
	}
}

void console::init()
{
	fb = framebuffer_request.response->framebuffers[0];

	for (size_t i = 0; i < ctx_count; i++)
		ft_ctxs[i] = make_ctx();

	active_ctx = 0;
	console_initialised = true;
}

void console::swap_ctx(size_t index)
{
	if (!console_initialised) return;
	if (index >= ctx_count) return;

	active_ctx = index;

	flanterm_full_refresh(ft_ctxs[active_ctx]);
}

void console::writes(const char* __restrict s)
{
	if (!console_initialised) return;
	
	while (*s)
	{
		writec(*s);
		s++;
	}
}

void console::writec(const char c)
{
	if (!console_initialised) return;
	
	flanterm_write(ft_ctxs[active_ctx], &c, 1);
}

int console::kprintf(const char* __restrict fmt, ...)
{
	print_timestamp();
	va_list args;
	va_start(args, fmt);
	int ret = vprintf(fmt, args);
	va_end(args);
	writes(ANSI_RESET "\r\n");

	return ret;
	return 0;
}

int console::kprintf_nv(const char* __restrict fmt, ...)
{
	print_timestamp();
	va_list args;
	va_start(args, fmt);
	int ret = vprintf(fmt, args);
	va_end(args);
	writes(ANSI_RESET "\r\n");

	return ret;
}

void console::print_timestamp()
{
	uint64_t total_microsecs = timers::total_us_elapsed();
	uint64_t secs = total_microsecs / 1000000;
	uint64_t microsecs = total_microsecs % 1000000;
	printf("[%llu.%06llu] ", secs, microsecs);	
}

void __putc__(char c)
{
	if (!console_initialised) return;
	
	flanterm_write(ft_ctxs[active_ctx], &c, 1);
}
