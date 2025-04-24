#pragma once
#include <stdbool.h>
#include <stddef.h>
typedef struct {
    const char** items;
    size_t len, cap;
} IncludeDirs;
typedef struct {
    const char* exe;
    const char* ipath;
    const char* opath;
    IncludeDirs includedirs;
} BuildOptions;
