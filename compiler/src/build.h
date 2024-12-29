#pragma once
#include "progstate.h"
typedef struct {
    size_t* items;
    size_t len, cap;
} BuildCallArgs;
enum {
    BUILD_LOAD_ARG,
    BUILD_ADD_INT,
    BUILD_RETURN,
    BUILD_ALLOCA,
    BUILD_LOAD_INT,
    BUILD_STORE_INT,
    BUILD_GET_ADDR_OF,
    BUILD_CALL_DIRECTLY,
    BUILD_CONST_INT,

    BUILD_INST_COUNT
};
typedef struct {
    int kind;
    union {
        size_t arg;        // Nth argument
        struct { 
            size_t v0, v1; // ID's of Instructions
        };
        struct {
            Type* type;
        };
        struct {
            Type* type;
            Atom* name;
        } func;
        struct {
            size_t globalid;
        };
        struct {
            size_t fid;
            BuildCallArgs args;
        } directcall;
        struct {
            Type* type;
            uint64_t value;
        } integer;
    };
} BuildInst;
typedef struct {
    BuildInst* items;
    size_t len, cap;
} Block;
enum {
    BUILD_SYM_ALLOC_INVALID,
    BUILD_SYM_ALLOC_DIRECT,
    BUILD_SYM_ALLOC_PTR,
    BUILD_SYM_ALLOC_COUNT,
};
typedef struct {
    size_t id;
    int allocation;
} BuildSymbol;
typedef struct {
    struct {
        Atom* name;
        BuildSymbol symbol;
    } *items;
    size_t len, cap;
} BuildSymbolTable;
enum {
    GLOBAL_ARRAY,
    GLOBAL_KIND_COUNT
};
typedef struct {
    uint8_t kind;
    union {
        struct { Type* typeid; void* data; size_t len; } array;
    };
} BuildGlobal;
typedef struct {
    size_t ip;
    struct {
        Block* items;
        size_t len, cap;
    } blocks;
    BuildSymbolTable local_table;
    Atom* name;
    Type* typeid;
} BuildFunc;
typedef struct {
    struct {
        BuildFunc* items;
        size_t len, cap;
    } funcs;
    struct {
        BuildGlobal* items;
        size_t len, cap;
    } globals;
    AtomTable* atom_table;
    TypeTable type_table;
    const char* path;
} Build;
typedef struct {
    Build* build;
    size_t fid;
    size_t head;
} BuildState;
void build_build(Build* build, ProgramState* state);
