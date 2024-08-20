#pragma once
#include <stdbool.h>
typedef struct {
    const char* exe;
    const char* ipath;
    const char* opath;
    bool experimental_windows;
} BuildOptions;
