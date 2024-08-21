#pragma once
#include "parser.h"
enum {
    BUILD_LOAD_ARG,
    BUILD_ADD_INT,
    BUILD_RETURN,
    BUILD_ALLOCA,
    BUILD_LOAD,
    BUILD_STORE,

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
            typeid_t type;
        };
    };
} BuildInst;
typedef struct {
    BuildInst* items;
    size_t len, cap;
} Block;
typedef struct {
    struct {
        Atom* name;
        size_t id;
    } *items;
    size_t len, cap;
} BuildSymbolTable;
typedef struct {
    size_t ip;
    struct {
       Block* items;
       size_t len, cap;
    } blocks;
    BuildSymbolTable local_table;
    Atom* name;
    typeid_t typeid;
} BuildFunc;
typedef struct {
    struct {
        BuildFunc* items;
        size_t len, cap;
    } funcs;
    AtomTable* atom_table;
    TypeTable type_table;
    const char* path;
} Build;
typedef struct {
    Build* build;
    size_t fid;
    size_t head;
} BuildState;
void build_build(Build* build, Parser* parser);
