#pragma once
#include <stdio.h>
#include "buildoptions.h"
#include "arena.h"

typedef enum {
    OUTPUT_UNDEFINED=0,
    OUTPUT_NASM,

    OUTPUT_COUNT
} OutputKind;
typedef enum {
    ARCH_UNDEFINED=0,
    ARCH_X86_64,

    ARCH_COUNT
} Architecture;
typedef enum {
    OS_UNDEFINED=0,
    OS_WINDOWS,
    OS_LINUX,

    PLATFORM_COUNT
} Platform;

typedef struct {
    const char* opath;
    OutputKind outputKind;
    Architecture arch;
    Platform platform;
} Target;

const char* outputkind_str(OutputKind kind);
const char* arch_str(Architecture kind);
const char* platform_str(Platform kind);
