#pragma once
#include "arena.h"
#include <stdint.h>
#include "atom.h"
#include "func.h"
enum {
    AST_CALL,
    AST_C_STR,
    AST_INT,
    AST_SYMBOL,
    AST_UNARY,
    AST_BINOP,
    AST_FUNC,
    AST_SUBSCRIPT,
    AST_NULL,
    AST_CAST,
    AST_KIND_COUNT
};
typedef struct AST AST;
typedef struct {
    AST** items;
    size_t len;
    size_t cap;
} CallArgs;
typedef struct Type Type;
typedef struct Symbol Symbol;
struct AST {
    AST* parent;
    int kind;
    Type* type;
    union {
        struct { int op; AST *lhs, *rhs; } binop;
        struct { int op; AST *rhs; } unary;
        struct { AST *what; CallArgs args; } call;
        struct { Type* type; uint64_t value; } integer;
        struct { const char* str; size_t len; } str;
        struct { Atom* name; Symbol* sym; } symbol;
        struct { AST *what, *with; } subscript;
        struct { AST *what; Type* into; } cast;
        Function* func;
    } as;
};
typedef struct {
    AST **items;
    size_t len, cap;
} ASTs;
AST* ast_new_binop(Arena* arena, int op, AST* lhs, AST* rhs);
AST* ast_new_symbol(Arena* arena, Atom* symbol);
AST* ast_new_cstr(Arena* arena, const char* str, size_t len);
AST* ast_new_int(Arena* arena, Type* type, uint64_t value);
AST* ast_new_unary(Arena* arena, int op, AST* rhs);
AST* ast_new_call(Arena* arena, AST* what, CallArgs args);
AST* ast_new_func(Arena* arena, Function* func);
AST* ast_new_subscript(Arena* arena, AST *what, AST* with);
AST* ast_new_null(Arena* arena);
AST* ast_new_cast(Arena* arena, AST *what, Type* into);
void call_args_dealloc(CallArgs* args);
