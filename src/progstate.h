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
            Atom* name;
            Symbol* symbol;
        } local_def;
    } as;
    /*metadata*/;
};
Statement* statement_return(Arena* arena, AST* ast);
Statement* statement_eval(Arena* arena, AST* ast);
Statement* statement_scope(Arena* arena);
Statement* statement_while(Arena* arena, AST* cond, Statement* body);
Statement* statement_loop(Arena* arena, Statement* body);
Statement* statement_local_def(Arena* arena, Atom* name, Symbol* symbol);
#include "func.h"
#include "syn_analys.h"
#include "constants.h"
typedef struct ProgramState ProgramState;
// TODO: For optimising iteration of functions, constants etc.
// Maybe we can just keep track of them in a list (i.e. array of Symbol*)
// That way the main way to lookup a symbol is through the symbol table
// but for actually iterating we can just go through a list hopefully
struct ProgramState {
    TypeTable type_table;
    SymTabNode symtab_root;
    Arena* arena;
};
