#pragma once
#include "type.h"
#include "ast.h"
enum {
    STATEMENT_RETURN,
    STATEMENT_EVAL,
    STATEMENT_COUNT
};
typedef struct Statement Statement;
typedef struct {
    Statement **items;
    size_t len, cap;
} Statements;
typedef struct Scope Scope;
struct Scope {
    Statements statements;
};
struct Statement {
    int kind;
    union {
        AST* ast;
    } as;
    /*metadata*/;
};
Statement* statement_return(Arena* arena, AST* ast);
Statement* statement_eval(Arena* arena, AST* ast);
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
