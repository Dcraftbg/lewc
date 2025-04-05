#include "ast.h"
#include "type.h"
#include <assert.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static AST* ast_new(Arena* arena, const Location* loc) {
    AST* ast = arena_alloc(arena, sizeof(*ast));
    assert(ast && "Ran out of memory");
    memset(ast, 0, sizeof(*ast));
    ast->loc = *loc;
    return ast;
}
AST* ast_new_binop(Arena* arena, const Location* loc, int op, AST* lhs, AST* rhs) {
    AST* ast = ast_new(arena, loc);
    lhs->parent = ast;
    rhs->parent = ast;
    ast->kind = AST_BINOP;
    ast->as.binop.op = op;
    ast->as.binop.lhs = lhs;
    ast->as.binop.rhs = rhs;
    return ast;
}
AST* ast_new_call(Arena* arena, const Location* loc, AST* what, CallArgs args) {
    AST* ast = ast_new(arena, loc);
    for(size_t i = 0; i < args.len; ++i) {
        args.items[i]->parent = ast;
    }
    ast->kind = AST_CALL;
    ast->as.call.what = what;
    ast->as.call.args = args;
    return ast;
}

AST* ast_new_int(Arena* arena, const Location* loc, Type* type, uint64_t value) {
    AST* ast = ast_new(arena, loc);
    ast->kind = AST_INT;
    ast->type = type;
    ast->as.integer.type  = type;
    ast->as.integer.value = value;
    return ast;
}

AST* ast_new_cstr(Arena* arena, const Location* loc, const char* str, size_t len) {
    AST* ast = ast_new(arena, loc);
    ast->kind = AST_C_STR;
    ast->type = &type_i8_ptr;
    ast->as.str.str = str;
    ast->as.str.len = len;
    return ast;
}
AST* ast_new_symbol(Arena* arena, const Location* loc, Atom* symbol) {
    AST* ast = ast_new(arena, loc);
    ast->kind = AST_SYMBOL;
    ast->as.symbol.name = symbol;
    ast->as.symbol.sym = NULL; // <- Defined in syntactical analysis
    return ast;
}

AST* ast_new_unary(Arena* arena, const Location* loc, int op, AST* rhs) {
    AST* ast = ast_new(arena, loc);
    rhs->parent = ast;
    ast->kind = AST_UNARY;
    ast->as.unary.op  = op;
    ast->as.unary.rhs = rhs;
    return ast;
}

AST* ast_new_func(Arena* arena, const Location* loc, Function* func) {
    AST* ast = ast_new(arena, loc);
    ast->kind = AST_FUNC;
    ast->type = func->type;
    ast->as.func = func;
    return ast;
}
AST* ast_new_subscript(Arena* arena, const Location* loc, AST *what, AST* with) {
    AST* ast = ast_new(arena, loc);
    ast->kind = AST_SUBSCRIPT;
    ast->as.subscript.what = what;
    ast->as.subscript.with = with;
    return ast;
}
AST* ast_new_null(Arena* arena, const Location* loc) {
    AST* ast = ast_new(arena, loc);
    ast->kind = AST_NULL;
    return ast;
}
AST* ast_new_cast(Arena* arena, const Location* loc, AST *what, Type* into) {
    AST* ast = ast_new(arena, loc);
    ast->kind = AST_CAST;
    ast->as.cast.what = what;
    ast->as.cast.into = into;
    return ast;
}
AST* ast_new_struct_literal(Arena* arena, const Location* loc, Type* type, StructLiteral literal) {
    AST* ast = ast_new(arena, loc);
    ast->kind = AST_STRUCT_LITERAL;
    ast->type = type;
    ast->as.struc_literal = literal;
    return ast;
}
void call_args_dealloc(CallArgs* this) {
    if(this->items) free(this->items);
    this->items = NULL;
    this->cap = 0;
    this->len = 0;
}
