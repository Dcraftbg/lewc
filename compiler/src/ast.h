#pragma once
#include "arena.h"
struct AST;
enum {
    AST_VALUE_SYMBOL,
    AST_VALUE_EXPR,
    AST_VALUE_C_STR,
    AST_VALUE_INT,

    AST_VALUE_COUNT,
};
typedef struct {
    int kind;
    union {
        struct AST* ast;
        struct { uint64_t value; } integer;
        struct { const char* str; size_t str_len; };
        Atom* symbol;
    };
} ASTValue;
enum {
    AST_SET=256,
    AST_CALL,
};
typedef struct {
    ASTValue* items;
    size_t len;
    size_t cap;
} CallArgs;
typedef struct AST {
    int kind;
    union {
        struct { ASTValue left, right; };
        struct { ASTValue what; CallArgs args; };
    };
} AST;
static AST* ast_new(Arena* arena, int kind, ASTValue left, ASTValue right) {
    AST* ast = (AST*)arena_alloc(arena, sizeof(*ast));
    if(!ast) return ast;
    ast->kind = kind;
    ast->left = left;
    ast->right = right;
    return ast;
}

static AST* ast_new_call(Arena* arena, ASTValue what, CallArgs args) {
    AST* ast = (AST*)arena_alloc(arena, sizeof(*ast));
    if(!ast) return ast;
    ast->kind = AST_CALL;
    ast->what = what;
    ast->args = args;
    return ast;
}
static void call_args_dealloc(CallArgs* this) {
    if(this->items) free(this->items);
    this->items = NULL;
    this->cap = 0;
    this->len = 0;
}
