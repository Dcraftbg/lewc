#pragma once
#include <stdio.h>
#include "buildoptions.h"
#include "arena.h"
typedef enum {
    BACKEND_UNDEFINED=0,
    BACKEND_QBE,
    
    BACKEND_COUNT
} Backend;
typedef enum {
    OUTPUT_UNDEFINED=0,
    OUTPUT_IR,
    OUTPUT_GAS,

    OUTPUT_COUNT
} OutputKind;
typedef enum {
    ARCH_UNDEFINED=0,
    ARCH_X86_64,

    ARCH_COUNT
} Architecture;
typedef enum {
    OS_UNDEFINED=0,
    OS_LINUX,

    PLATFORM_COUNT
} Platform;

typedef struct {
    OutputKind outputKind;
    Architecture arch;
    Platform platform;
    Backend backend;
} Target;

const char* outputkind_str(OutputKind kind);
const char* arch_str(Architecture kind);
const char* platform_str(Platform kind);
const char* backend_str(Backend backend);
