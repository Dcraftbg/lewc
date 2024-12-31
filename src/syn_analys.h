#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "arena.h"
#include "atom.h"
#include "type.h"

typedef struct SymTabNode SymTabNode;
// TODO: Keep track of the type of symbol.
// i.e. Function, variable etc.
// And checks for trying to assign to a function and so on
typedef struct {
    Type* type;
} Symbol;
#ifdef SYMTAB_DEFINE
#define HASHMAP_DEFINE
#endif
#include "hashmap.h"
#define SYM_TAB_ALLOC(n) malloc(n)
#define SYM_TAB_DEALLOC(ptr, size) free(ptr)
MAKE_HASHMAP_EX(SymTab, sym_tab, Symbol*, Atom*, atom_hash, atom_eq, SYM_TAB_ALLOC, SYM_TAB_DEALLOC)
#ifdef SYMTAB_DEFINE
#undef HASHMAP_DEFINE
#endif
struct SymTabNode {
    SymTabNode* parent; 
    SymTab symtab;
    // NOTE: Children
    SymTabNode** items;
    size_t   len, cap;
};
typedef struct ProgramState ProgramState;

Symbol* stl_lookup(SymTabNode* node, Atom* a);
bool syn_analyse(ProgramState* state);
