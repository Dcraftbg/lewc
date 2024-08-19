#pragma once
#include "arena.h"
struct AST;
enum {
    AST_VALUE_SYMBOL,
    AST_VALUE_EXPR,

    AST_VALUE_COUNT,
};
typedef struct {
    int kind;
    union {
        struct AST* ast;
        uint64_t integer;
        Atom* symbol;
    };
} ASTValue;
enum {
    AST_SET=256,
};
typedef struct AST {
    int kind;
    ASTValue left, right;
} AST;
static AST* ast_new(Arena* arena, int kind, ASTValue left, ASTValue right) {
    AST* ast = (AST*)arena_alloc(arena, sizeof(*ast));
    if(!ast) return ast;
    ast->kind = kind;
    ast->left = left;
    ast->right = right;
    return ast;
}
