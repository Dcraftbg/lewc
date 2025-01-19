#include "ast.h"
#include <assert.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static AST* ast_new(Arena* arena) {
    AST* ast = arena_alloc(arena, sizeof(*ast));
    assert(ast && "Ran out of memory");
    memset(ast, 0, sizeof(*ast));
    return ast;
}
AST* ast_new_binop(Arena* arena, int kind, AST* lhs, AST* rhs) {
    AST* ast = ast_new(arena);
    ast->kind = kind;
    ast->as.binop.lhs = lhs;
    ast->as.binop.rhs = rhs;
    return ast;
}
AST* ast_new_call(Arena* arena, AST* what, CallArgs args) {
    AST* ast = ast_new(arena);
    ast->kind = AST_CALL;
    ast->as.call.what = what;
    ast->as.call.args = args;
    return ast;
}

AST* ast_new_int(Arena* arena, uint64_t value) {
    AST* ast = ast_new(arena);
    ast->kind = AST_INT;
    ast->as.integer.value = value;
    return ast;
}

AST* ast_new_cstr(Arena* arena, const char* str, size_t len) {
    AST* ast = ast_new(arena);
    ast->kind = AST_C_STR;
    ast->as.str.str = str;
    ast->as.str.len = len;
    return ast;
}
AST* ast_new_symbol(Arena* arena, Atom* symbol) {
    AST* ast = ast_new(arena);
    ast->kind = AST_SYMBOL;
    ast->as.symbol = symbol;
    return ast;
}
AST* ast_new_deref(Arena* arena, AST* what) {
    AST* ast = ast_new(arena);
    ast->kind = AST_DEREF;
    ast->as.deref.what = what;
    return ast;
}

void call_args_dealloc(CallArgs* this) {
    if(this->items) free(this->items);
    this->items = NULL;
    this->cap = 0;
    this->len = 0;
}
