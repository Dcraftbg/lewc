#pragma once
#include "arena.h"
#include "location.h"
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
    AST_STRUCT_LITERAL,
    AST_KIND_COUNT
};
typedef struct AST AST;
typedef struct {
    AST** items;
    size_t len;
    size_t cap;
} CallArgs;
typedef struct {
    Atom* name;
    AST* value;
} StructLiteralField;
typedef struct {
    StructLiteralField *items;
    size_t len, cap;
} StructLiteralFields;
typedef struct {
    StructLiteralFields fields;
} StructLiteral;
typedef struct Type Type;
typedef struct Symbol Symbol;
struct AST {
    AST* parent;
    Location loc;
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
        StructLiteral struc_literal;
        Function* func;
    } as;
};
typedef struct {
    AST **items;
    size_t len, cap;
} ASTs;
AST* ast_new_binop(Arena* arena, const Location* loc, int op, AST* lhs, AST* rhs);
AST* ast_new_symbol(Arena* arena, const Location* loc, Atom* symbol);
AST* ast_new_cstr(Arena* arena, const Location* loc, const char* str, size_t len);
AST* ast_new_int(Arena* arena, const Location* loc, Type* type, uint64_t value);
AST* ast_new_unary(Arena* arena, const Location* loc, int op, AST* rhs);
AST* ast_new_call(Arena* arena, const Location* loc, AST* what, CallArgs args);
AST* ast_new_func(Arena* arena, const Location* loc, Function* func);
AST* ast_new_subscript(Arena* arena, const Location* loc, AST *what, AST* with);
AST* ast_new_null(Arena* arena, const Location* loc);
AST* ast_new_cast(Arena* arena, const Location* loc, AST *what, Type* into);
AST* ast_new_struct_literal(Arena* arena, const Location* loc, Type* type, StructLiteral literal);
void call_args_dealloc(CallArgs* args);
