#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "arena.h"
#include "atom.h"
#include "type.h"
#include "constants.h"

typedef struct SymTabNode SymTabNode;
typedef struct Symbol Symbol;
typedef struct {
    AST **items;
    size_t len, cap;
} ASTs;
struct Symbol {
    Type* type;
    enum {
        SYMBOL_CONSTANT,
        SYMBOL_VARIABLE,
        SYMBOL_COUNT
    } kind;
    AST* ast;
    bool evaluated;
    ASTs infer_asts;
};

Symbol* symbol_new_var(Arena* arena, Type* type, AST* init);
Symbol* symbol_new_constant(Arena* arena, Type* type, AST* init);
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

bool syn_analyse(ProgramState* state);
