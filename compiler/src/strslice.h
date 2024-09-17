#pragma once
#include "hashutils.h"
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
typedef struct {
    const char* data;
    size_t len;
} StrSlice;
#define strslice_hash(s) djb2((s).data, (s).len)
static inline bool strslice_eq(StrSlice s1, StrSlice s2) {
    return s1.len == s2.len && (memcmp(s1.data, s2.data, s1.len) == 0);
}
static inline StrSlice strslice_cstr(const char* cstr) {
    return (StrSlice) { .data = cstr, .len = strlen(cstr) };
}
#define STRSLICE_CSTR_CONST(lit) (StrSlice){.data=(cstr), .len=sizeof(cstr)}
