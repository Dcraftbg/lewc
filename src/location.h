#pragma once
#include <stddef.h>
typedef struct {
    const char* path;
    size_t l0, c0;
    size_t l1, c1;
    // Pointer to the original token source string
    const char *src;
    size_t src_len;
} Location;
const char* tloc(const Location* loc);
Location loc_join(const Location* a, const Location* b);
