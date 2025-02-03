#include "utf8.h"
const char* utf8_end(const char* str) {
	unsigned char c = *((const unsigned char*)str);
	if (c < 0x80) return str + 1;
	if ((c & 0xE0) == 0xC0) return str + 2;
	if ((c & 0xF0) == 0xE0) return str + 3;
	if ((c & 0xF8) == 0xF0) return str + 4;
	return NULL; // Invalid characterd
}
uint32_t utf8_next(const char** stream, const char* end) { // -1 for error
	const char* u_end = utf8_end(*stream);
	if (u_end > end) return -1;
	uint32_t c = 0;
	while (*stream < u_end) {
		c = c << 8 | **stream;
		(*stream)++;
	}
	return c;
}
