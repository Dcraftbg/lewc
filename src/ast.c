#include "ast.h"
#include "type.h"
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
AST* ast_new_binop(Arena* arena, int op, AST* lhs, AST* rhs) {
    AST* ast = ast_new(arena);
    lhs->parent = ast;
    rhs->parent = ast;
    ast->kind = AST_BINOP;
    ast->as.binop.op = op;
    ast->as.binop.lhs = lhs;
    ast->as.binop.rhs = rhs;
    return ast;
}
AST* ast_new_call(Arena* arena, AST* what, CallArgs args) {
    AST* ast = ast_new(arena);
    for(size_t i = 0; i < args.len; ++i) {
        args.items[i]->parent = ast;
    }
    ast->kind = AST_CALL;
    ast->as.call.what = what;
    ast->as.call.args = args;
    return ast;
}

AST* ast_new_int(Arena* arena, Type* type, uint64_t value) {
    AST* ast = ast_new(arena);
    ast->kind = AST_INT;
    ast->type = type;
    ast->as.integer.type  = type;
    ast->as.integer.value = value;
    return ast;
}

AST* ast_new_cstr(Arena* arena, const char* str, size_t len) {
    AST* ast = ast_new(arena);
    ast->kind = AST_C_STR;
    ast->type = &type_u8_ptr;
    ast->as.str.str = str;
    ast->as.str.len = len;
    return ast;
}
AST* ast_new_symbol(Arena* arena, Atom* symbol) {
    AST* ast = ast_new(arena);
    ast->kind = AST_SYMBOL;
    ast->as.symbol.name = symbol;
    ast->as.symbol.sym = NULL; // <- Defined in syntactical analysis
    return ast;
}

AST* ast_new_unary(Arena* arena, int op, AST* rhs) {
    AST* ast = ast_new(arena);
    rhs->parent = ast;
    ast->kind = AST_UNARY;
    ast->as.unary.op  = op;
    ast->as.unary.rhs = rhs;
    return ast;
}

AST* ast_new_func(Arena* arena, Function* func) {
    AST* ast = ast_new(arena);
    ast->kind = AST_FUNC;
    ast->type = func->type;
    ast->as.func = func;
    return ast;
}
void call_args_dealloc(CallArgs* this) {
    if(this->items) free(this->items);
    this->items = NULL;
    this->cap = 0;
    this->len = 0;
}
