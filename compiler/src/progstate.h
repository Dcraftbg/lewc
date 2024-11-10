#pragma once
#include "type.h"
#include "ast.h"
enum {
    STATEMENT_RETURN,
    STATEMENT_EVAL,
    STATEMENT_COUNT
};
typedef struct {
    int kind;
    union {
        AST* ast;
    };
    /*metadata*/;
} Statement;
enum {
    SCOPE_GLOBAL,
    SCOPE_FUNC
};
typedef struct {
    Statement *items;
    size_t len, cap;
} Statements;
typedef struct Scope {
    struct Scope* parent;
    int kind;
    Statements statements;
} Scope;
typedef struct {
    typeid_t type;
    Scope* scope;
} Function;
#ifdef FUNC_MAP_DEFINE
#define HASHMAP_DEFINE
#endif
#include "hashmap.h"
#define FUNC_MAP_ALLOC(n) malloc(n)
#define FUNC_MAP_DEALLOC(ptr, size) free(ptr)
MAKE_HASHMAP_EX(FuncMap, func_map, Function, Atom*, atom_hash, atom_eq, FUNC_MAP_ALLOC, FUNC_MAP_DEALLOC)
#ifdef FUNC_MAP_DEFINE
#undef HASHMAP_DEFINE
#endif
typedef struct {
    FuncMap map;
} Funcs;

typedef struct {
    TypeTable type_table;
    Scope global;
    Funcs funcs;
} ProgramState;
