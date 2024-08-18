#pragma once
#include "parser.h"
enum {
    BUILD_LOAD_ARG,
    BUILD_ADD_INT_SIGNED,
    BUILD_RETURN,


    BUILD_INST_COUNT
};
typedef struct {
    int kind;
    union {
        size_t arg;        // Nth argument
        struct { 
            size_t v0, v1; // ID's of Instructions
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
} Build;
typedef struct {
    Build* build;
    size_t fid;
    size_t head;
} BuildState;
void build_build(Build* build, Parser* parser);
