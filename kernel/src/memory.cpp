#include <cstdint>
#include <cstddef>
#include <utils.hpp>

// GCC and Clang reserve the right to generate calls to the following
// 4 C-linkage functions even if they are not directly called.
// They must be implemented as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!

extern "C" {

void *memcpy(void *__restrict dest, const void *__restrict src, size_t n) {
    uint8_t *__restrict pdest = static_cast<uint8_t *__restrict>(dest);
    const uint8_t *__restrict psrc = static_cast<const uint8_t *__restrict>(src);

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = static_cast<uint8_t *>(s);

    for (size_t i = 0; i < n; i++) {
        p[i] = static_cast<uint8_t>(c);
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = static_cast<uint8_t *>(dest);
    const uint8_t *psrc = static_cast<const uint8_t *>(src);

    if (reinterpret_cast<uintptr_t>(src) > reinterpret_cast<uintptr_t>(dest)) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (reinterpret_cast<uintptr_t>(src) < reinterpret_cast<uintptr_t>(dest)) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = static_cast<const uint8_t *>(s1);
    const uint8_t *p2 = static_cast<const uint8_t *>(s2);

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

}
