#pragma once
#include <stddef.h>
static size_t djb2(const char *str, size_t len) {
    size_t hash = 5381;
    for(size_t i = 0; i < len; ++i) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}
