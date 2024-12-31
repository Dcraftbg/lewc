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

#include "func.h"
#include "syn_analys.h"
typedef struct ProgramState ProgramState;
struct ProgramState {
    TypeTable type_table;
    Scope global;
    FuncMap funcs;
    SymTabNode symtab_root;
    Arena* arena;
};
