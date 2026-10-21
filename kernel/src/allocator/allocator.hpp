#pragma once

#include <cstddef>
#include <cstdint>

namespace alloc
{
	void init(void* base, size_t allocated_pages);
	void* malloc(size_t size);
	void* calloc(size_t nmemb, size_t size);
	void* realloc(void* ptr, size_t size);
	void free(void* ptr);
}
