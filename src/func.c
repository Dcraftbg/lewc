#include "func.h"
#include <assert.h>
Function* func_new(Arena* arena, Type* type, Statements* scope) {
    Function* me = arena_alloc(arena, sizeof(*me));
    assert(me && "Ran out of memory");
    memset(me, 0, sizeof(*me));
    me->type = type;
    me->scope = scope;
    return me;
}
