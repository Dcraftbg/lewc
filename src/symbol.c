#include "symbol.h"
static Symbol* symbol_new(Arena* arena, Type* type) {
    Symbol* symbol = arena_alloc(arena, sizeof(*symbol));
    assert(symbol && "Ran out of memory");
    memset(symbol, 0, sizeof(*symbol));
    symbol->type = type;
    return symbol;
}
Symbol* symbol_new_var(Arena* arena, Type* type, AST* init) {
    Symbol* me = symbol_new(arena, type);
    if(!me) return NULL;
    me->kind = SYMBOL_VARIABLE;
    me->ast = init;
    return me;
}
Symbol* symbol_new_constant(Arena* arena, Type* type, AST* init) {
    Symbol* me = symbol_new(arena, type);
    if(!me) return NULL;
    me->kind = SYMBOL_CONSTANT;
    me->ast = init;
    return me;
}
