#pragma once

#include "token.h"
#include "lexer.h"
#include "type.h"
#include "symbol.h"
#include "ast.h"
#include "buildoptions.h"
extern BuildOptions build_options;
static void _example_dump() {
    Arena arena={0};
    AtomTable atom_table={0};
    Lexer lexer;
    lexer_create(&lexer, build_options.ipath, &atom_table, &arena);
    while(lexer_peak_next(&lexer).kind != TOKEN_EOF) {
        Token t = lexer_next(&lexer);
        if(t.kind >= TOKEN_END) {
            eprintfln("ERROR:%s: Lexer: %s", tloc(t), tdisplay(t));
            exit(1);
        }
        printf("%s: Gotten token: %s\n", tloc(t), tdisplay(t));
    }
    lexer_cleanup(&lexer);
}
enum {
    INST_RETURN,
    INST_EVAL,
    INST_COUNT
};
typedef struct {
    int kind;
    union {
        ASTValue astvalue;
    };
    /*metadata*/;
} Instruction;
enum {
    SCOPE_GLOBAL,
    SCOPE_FUNC
};
typedef struct {
    Instruction *items;
    size_t len, cap;
} Insts;
typedef struct Scope {
    struct Scope* parent;
    int kind;
    SymbolTable symtab; 
    Insts insts;
} Scope;
struct FuncPair {
    Atom* name;
    typeid_t type;
    Scope* scope;
};
typedef struct {
    struct FuncPair *items;
    size_t len, cap;
} Funcs;
typedef struct {
    Arena* arena;
    TypeTable type_table;
    Lexer* lexer;
    Scope global;
    Scope* head;
    Funcs funcs;
} Parser;
void parser_create(Parser* this, Lexer* lexer, Arena* arena);
void parse(Parser* parser, Lexer* lexer, Arena* arena);
