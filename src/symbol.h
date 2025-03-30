#pragma once
#include "type.h"
#include "ast.h"
typedef struct Symbol Symbol;
struct Symbol {
    Type* type;
    enum {
        SYMBOL_CONSTANT,
        SYMBOL_VARIABLE,
        SYMBOL_COUNT
    } kind;
    AST* ast;
    bool evaluated;
    ASTs infer_asts;
};

Symbol* symbol_new_var(Arena* arena, Type* type, AST* init);
Symbol* symbol_new_constant(Arena* arena, Type* type, AST* init);
