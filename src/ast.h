#pragma once
#include "arena.h"
#include <stdint.h>
#include "atom.h"
enum {
    AST_SET,
    AST_CALL,
    AST_C_STR,
    AST_INT,
    AST_SYMBOL,
    AST_DEREF,
    AST_BINOP,
    AST_KIND_COUNT
};
typedef struct AST AST;
typedef struct {
    AST** items;
    size_t len;
    size_t cap;
} CallArgs;
typedef struct Type Type;
struct AST {
    int kind;
    Type* type;
    union {
        struct { int op; AST *lhs, *rhs; } binop;
        struct { AST *what; } deref;
        struct { AST *what; CallArgs args; } call;
        struct { uint64_t value; } integer;
        struct { const char* str; size_t len; } str;
        Atom* symbol;
    } as;
};

AST* ast_new_binop(Arena* arena, int op, AST* lhs, AST* rhs);
AST* ast_new_symbol(Arena* arena, Atom* symbol);
AST* ast_new_cstr(Arena* arena, const char* str, size_t len);
AST* ast_new_int(Arena* arena, uint64_t value);
AST* ast_new_deref(Arena* arena, AST* what);
AST* ast_new_call(Arena* arena, AST* what, CallArgs args);
void call_args_dealloc(CallArgs* args);
