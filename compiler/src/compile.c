#include "compile.h"
#include "assert.h"
#include "utils.h"
static_assert(OUTPUT_COUNT == 1, "Update outputkind_map");
const char* outputkind_map[OUTPUT_COUNT] = {
    "Nasm Assembly" 
};
const char* outputkind_str(OutputKind kind) {
    assert(kind >= 0 && kind < ARRAY_LEN(outputkind_map));
    return outputkind_map[kind];
}

static_assert(ARCH_COUNT == 1, "Update arch_map");
const char* arch_map[ARCH_COUNT] = {
    "x86_64" 
};
const char* arch_str(Architecture kind) {
    assert(kind >= 0 && kind < ARRAY_LEN(arch_map));
    return arch_map[kind];
}

static_assert(PLATFORM_COUNT == 2, "Update platform_map");
const char* platform_map[PLATFORM_COUNT] = {
    [OS_WINDOWS] = "Windows",
    [OS_LINUX] = "Linux"
};
const char* platform_str(Platform kind) {
    assert(kind >= 0 && kind < ARRAY_LEN(platform_map));
    return platform_map[kind];
}

void compile(Build* build, Target* target, Arena* arena) {
    static_assert(OUTPUT_COUNT == 1, "Update compile");
    CompileState state = {0};
    state.target = target;
    state.build = build;
    state.arena = arena;
    if(target->arch == ARCH_X86_64 && target->platform == OS_WINDOWS) {
        compile_nasm_x86_64_windows(&state);
    } else if (target->arch == ARCH_X86_64 && target->platform == OS_LINUX) {
        compile_nasm_x86_64_linux(&state);
    } else 
    {
        eprintfln("Unsupported target %s:%s outputing to %s", platform_str(target->platform), arch_str(target->arch), outputkind_str(target->outputKind));
        exit(1);
    }
}
