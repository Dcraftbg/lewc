#pragma once
#include "ast.h"
// TODO: Keep track of location
typedef struct {
    bool evaluated;
    AST* ast;
    Type* type;
} Constant;
#ifdef CONSTTAB_DEFINE
#   define HASHMAP_DEFINE
#endif
#include "hashmap.h"
#define CONSTTAB_ALLOC(n) malloc(n)
#define CONSTTAB_DEALLOC(ptr, size) ((void)(size), free(ptr))
MAKE_HASHMAP_EX(ConstTab, const_tab, Constant*, Atom*, atom_hash, atom_eq, CONSTTAB_ALLOC, CONSTTAB_DEALLOC)
#ifdef CONSTTAB_DEFINE
#   undef HASHMAP_DEFINE
#endif
