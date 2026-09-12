/* Extend Tibia's memcpy/memset runtime with the bypass copy operation. */
#ifndef ASID_WASM_STRING_H
#define ASID_WASM_STRING_H
#include_next <string.h>
#include <stdint.h>

static inline void *memmove(void *dest, const void *src, size_t size) {
	unsigned char *d = (unsigned char *)dest;
	const unsigned char *s = (const unsigned char *)src;
	if ((uintptr_t)d < (uintptr_t)s) {
		for (size_t i = 0; i < size; i++) d[i] = s[i];
	} else if (d != s) {
		while (size) { size--; d[size] = s[size]; }
	}
	return dest;
}
#endif
