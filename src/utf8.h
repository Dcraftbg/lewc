#pragma once
#include <stdint.h>
#include <stddef.h>
const char* utf8_end(const char* str);
uint32_t utf8_next(const char** stream, const char* end);
