#pragma once
#include "atom.h"
typedef struct Type Type;
typedef struct {
    Atom* name; // NULL if no name is specified
    Type* type;
} Arg;
typedef struct {
    Arg *items;
    size_t len, cap;
} Args;
typedef struct {
    Args input;
    Type* output;
} FuncSignature;
typedef struct Scope Scope;
typedef struct {
    Type* type;
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
