#pragma once

#include "token.h"
#include "lexer.h"
#include "type.h"
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

#include "progstate.h"
typedef struct {
    Arena* arena;
    Lexer* lexer;
    Scope* head;
    ProgramState* state;
} Parser;
void parser_create(Parser* this, Lexer* lexer, Arena* arena, ProgramState* state);
void parse(Parser* parser, Lexer* lexer, Arena* arena);
