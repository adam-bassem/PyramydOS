#pragma once

#include "printf.h"

#include <cstddef>

#define ANSI_RESET        "\033[0m"
#define ANSI_BOLD         "\033[1m"

#define ANSI_BLACK        "\033[30m"
#define ANSI_RED          "\033[31m"
#define ANSI_GREEN        "\033[32m"
#define ANSI_YELLOW       "\033[33m"
#define ANSI_BLUE         "\033[34m"
#define ANSI_PURPLE       "\033[35m"
#define ANSI_CYAN         "\033[36m"
#define ANSI_WHITE        "\033[37m"

#define ANSI_BG_BLACK     "\033[40m"
#define ANSI_BG_RED       "\033[41m"
#define ANSI_BG_GREEN     "\033[42m"
#define ANSI_BG_YELLOW    "\033[43m"
#define ANSI_BG_BLUE      "\033[44m"
#define ANSI_BG_PURPLE    "\033[45m"
#define ANSI_BG_CYAN      "\033[46m"
#define ANSI_BG_WHITE     "\033[47m"

namespace console
{
	void init();

	void writes(const char* __restrict s);
	void writec(const char c);	

	void swap_ctx(size_t index);

	int kprintf(const char* __restrict fmt, ...);
	void print_timestamp();
}

extern "C" void __putc__(char c);
