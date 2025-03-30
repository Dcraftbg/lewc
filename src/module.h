#pragma once
#include "type.h"
#include "ast.h"
#include "func.h"
#include "syn_analys.h"
#include "constants.h"
typedef struct Module Module;

struct Module {
    const char* path;
    TypeTable type_table;
    SymTabNode symtab_root;
    struct {
        Symbol** items;
        size_t len, cap;
    } constants;
    Arena* arena;
};
Module* module_new(Arena* arena, const char* path);
