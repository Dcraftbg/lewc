#include "progstate.h"
Statement* statement_new(Arena* arena) {
    Statement* st = arena_alloc(arena, sizeof(*st));
    assert(st && "Ran out of memory");
    memset(st, 0, sizeof(*st));
    return st;
}
Statement* statement_return(Arena* arena, AST* ast) {
    Statement* me = statement_new(arena);
    me->kind = STATEMENT_RETURN;
    me->as.ast = ast;
    return me;
}
Statement* statement_eval(Arena* arena, AST* ast) {
    Statement* me = statement_new(arena);
    me->kind = STATEMENT_EVAL;
    me->as.ast = ast;
    return me;
}
