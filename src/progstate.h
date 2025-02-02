#pragma once
#include "type.h"
#include "ast.h"
typedef struct Statement Statement;
typedef struct Statements Statements;
struct Statements {
    Statement **items;
    size_t len, cap;
};
Statements* scope_new(Arena* arena);
enum {
    STATEMENT_RETURN,
    STATEMENT_EVAL,
    STATEMENT_SCOPE,
    STATEMENT_WHILE,
    STATEMENT_LOOP,
    STATEMENT_LOCAL_DEF,
    STATEMENT_COUNT
};
struct Statement {
    int kind;
    union {
        AST* ast;
        Statements* scope;
        struct { 
            AST* cond;
            Statement* body;
        } whil;
        struct {
            Statement* body;
        } loop;
        struct { 
            Atom* name;
            Type* type;
            AST * init;
        } local_def;
    } as;
    /*metadata*/;
};
Statement* statement_return(Arena* arena, AST* ast);
Statement* statement_eval(Arena* arena, AST* ast);
Statement* statement_scope(Arena* arena);
Statement* statement_while(Arena* arena, AST* cond, Statement* body);
Statement* statement_loop(Arena* arena, Statement* body);
Statement* statement_local_def(Arena* arena, Atom* name, Type* type, AST* init);
#include "func.h"
#include "syn_analys.h"
#include "constants.h"
typedef struct ProgramState ProgramState;
struct ProgramState {
    TypeTable type_table;
    FuncMap funcs;
    ConstTab consts;
    SymTabNode symtab_root;
    Arena* arena;
};
