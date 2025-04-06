#include "symbol.h"
static Symbol* symbol_new(Arena* arena, const Location* loc, Type* type) {
    Symbol* symbol = arena_alloc(arena, sizeof(*symbol));
    assert(symbol && "Ran out of memory");
    memset(symbol, 0, sizeof(*symbol));
    symbol->type = type;
    symbol->loc = *loc;
    return symbol;
}
Symbol* symbol_new_global(Arena* arena, const Location* loc, Type* type, AST* init) {
    Symbol* me = symbol_new(arena, loc, type);
    if(!me) return NULL;
    me->kind = SYMBOL_GLOBAL;
    me->ast = init;
    return me;
}
Symbol* symbol_new_var(Arena* arena, const Location* loc, Type* type, AST* init) {
    Symbol* me = symbol_new(arena, loc, type);
    if(!me) return NULL;
    me->kind = SYMBOL_VARIABLE;
    me->ast = init;
    return me;
}
Symbol* symbol_new_constant(Arena* arena, const Location* loc, Type* type, AST* init) {
    Symbol* me = symbol_new(arena, loc, type);
    if(!me) return NULL;
    me->kind = SYMBOL_CONSTANT;
    me->ast = init;
    return me;
}
