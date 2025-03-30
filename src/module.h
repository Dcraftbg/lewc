#pragma once
#include "type.h"
#include "ast.h"
#include "func.h"
#include "syn_analys.h"
#include "constants.h"
typedef struct Module Module;

typedef struct {
    Symbol* symbol;
    Atom* name;
} ModuleSymbol;
typedef struct {
    ModuleSymbol* items;
    size_t len, cap;
} ModuleSymbols;
struct Module {
    const char* path;
    TypeTable type_table;
    SymTabNode symtab_root;
    ModuleSymbols symbols;
    Arena* arena;
};
Module* module_new(Arena* arena, const char* path);
bool module_do_intermediate_steps(Module* module);
