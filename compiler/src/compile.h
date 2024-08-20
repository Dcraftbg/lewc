#pragma once
#include "build.h"
#include "arena.h"
#include "arch/x86_64/nasm.h"
#include "arch/x86_64/windows/windows.h"
typedef enum {
    OUTPUT_NASM,

    OUTPUT_COUNT
} OutputKind;
typedef enum {
    ARCH_X86_64,

    ARCH_COUNT
} Architecture;
typedef enum {
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

typedef struct CompileState {
    Build* build;
    Target* target;
    BuildOptions* options;
    FILE* f;
    const char* srcfile;
    Arena* arena;
    NasmGPRegsAlloc regs;
} CompileState;

const char* outputkind_str(OutputKind kind);
const char* arch_str(Architecture kind);
const char* platform_str(Platform kind);
void compile(Build* build, Target* target, Arena* arena); 
