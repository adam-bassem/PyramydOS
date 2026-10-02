#pragma once

#include "printf.h"

#include <cstddef>

namespace console
{
	void init();

	void writes(const char* __restrict s);
	void writec(const char c);	

	void swap_ctx(size_t index);

	int kprintf(const char* __restrict fmt, ...);
}

extern "C" void __putc__(char c);
