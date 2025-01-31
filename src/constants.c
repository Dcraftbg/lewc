#define CONSTTAB_DEFINE
#include "constants.h"

Constant* const_new(Arena* arena, AST* ast, Type* type) {
    Constant* me = arena_alloc(arena, sizeof(*me));
    memset(me, 0, sizeof(*me));
    me->evaluated = false;
    me->ast = ast;
    me->type = type;
    return me;
}
