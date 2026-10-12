#include "console.hpp"
#include <utils.hpp>
#include <allocator/allocator.hpp>
#include <timers/timers.hpp>
#include <cstdint>
#include <console/term/term.hpp>

namespace
{
	bool console_initialised = false;
	struct limine_framebuffer* fb;
}

void console::init()
{
	console_initialised = true;
	console::term::init();
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
	
	console::term::putc(c);
}

int console::kprintf(const char* __restrict fmt, ...)
{
	if (!console_initialised) return -1;

	print_timestamp();
	va_list args;
	va_start(args, fmt);
	int ret = vprintf(fmt, args);
	va_end(args);
	writes(ANSI_RESET "\r\n");

	return ret;
	return 0;
}

void console::print_timestamp()
{
	if (!console_initialised) return;

	uint64_t total_microsecs = timers::total_us_elapsed();
	uint64_t secs = total_microsecs / 1000000;
	uint64_t microsecs = total_microsecs % 1000000;
	printf("[%llu.%06llu] ", secs, microsecs);	
}

void __putc__(char c)
{
	if (!console_initialised) return;

	console::term::putc(c);
}
