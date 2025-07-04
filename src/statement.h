#pragma once
#include <stdbool.h>
#include <stddef.h>
typedef struct AST AST;
typedef struct Atom Atom;
typedef struct Arena Arena;
typedef struct Symbol Symbol;
typedef struct Statement Statement;
typedef struct Statements Statements;
struct Statements {
    bool terminal;
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
    STATEMENT_IF,
    STATEMENT_LOCAL_DEF,
    STATEMENT_DEFER,
    STATEMENT_COUNT
};
struct Statement {
    bool terminal;
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
            AST* cond;
            Statement* body;
            Statement* elze;
        } iff;
        struct {
            Atom* name;
            Symbol* symbol;
        } local_def;
        struct {
            Statement* statement;
        } defer;
    } as;
    /*metadata*/;
};
Statement* statement_return(Arena* arena, AST* ast);
Statement* statement_eval(Arena* arena, AST* ast);
Statement* statement_scope(Arena* arena);
Statement* statement_if(Arena* arena, AST* cond, Statement* body, Statement* elze);
Statement* statement_while(Arena* arena, AST* cond, Statement* body);
Statement* statement_loop(Arena* arena, Statement* body);
Statement* statement_local_def(Arena* arena, Atom* name, Symbol* symbol);
Statement* statement_defer(Arena* arena, Statement* statement);
