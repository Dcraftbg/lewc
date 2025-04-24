#include "compile.h"
#include "assert.h"
#include "utils.h"

static_assert(BACKEND_COUNT == 2, "Update backend_map");
const char* backend_map[BACKEND_COUNT] = {
    [BACKEND_UNDEFINED] = "Undefined",
    [BACKEND_QBE] = "qbe" 
};
const char* backend_str(Backend backend) {
    assert(backend >= 0 && backend < ARRAY_LEN(backend_map));
    return backend_map[backend];
}

static_assert(OUTPUT_COUNT == 4, "Update outputkind_map");
const char* outputkind_map[OUTPUT_COUNT] = {
    "Undefined",
    [OUTPUT_IR]  = "Intermediate Representation",
    [OUTPUT_GAS] = "GNU Assembly",
    [OUTPUT_OBJ] = "Object File",
};
const char* outputkind_str(OutputKind kind) {
    assert(kind >= 0 && kind < ARRAY_LEN(outputkind_map));
    return outputkind_map[kind];
}

static_assert(ARCH_COUNT == 2, "Update arch_map");
const char* arch_map[ARCH_COUNT] = {
    "Undefined",
    "x86_64" 
};
const char* arch_str(Architecture kind) {
    assert(kind >= 0 && kind < ARRAY_LEN(arch_map));
    return arch_map[kind];
}

static_assert(PLATFORM_COUNT == 2, "Update platform_map");
const char* platform_map[PLATFORM_COUNT] = {
    [OS_UNDEFINED] = "Undefined",
    [OS_LINUX] = "Linux"
};
const char* platform_str(Platform kind) {
    assert(kind >= 0 && kind < ARRAY_LEN(platform_map));
    return platform_map[kind];
}

