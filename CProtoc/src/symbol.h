#pragma once
#include <stddef.h>
#include "atom.h"
#include "type.h"
#include <string.h>

enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNC,
};
typedef struct {
    int symbol_type;
    typeid_t typeid;
} Symbol;
typedef struct {
    struct {
        Atom* name;
        Symbol* symbol;
    } *items;
    size_t len, cap;
} SymbolTable;

// TODO: change out strcmp for == once atoms are UNIQUE identifiers, meaning 
// you can compare them as pointers. They lie in the same AtomTable so its ok
Symbol* symtab_symbol_find(SymbolTable* table, Atom* name);
void symtab_insert(SymbolTable* table, Atom* name, Symbol* symbol);
