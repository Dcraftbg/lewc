#include "module.h"
Module* module_new(Arena* arena, const char* path) {
    Module* me = arena_alloc(arena, sizeof(Module));
    assert(me && "Ran out of memory");
    memset(me, 0, sizeof(*me));
    me->path = path;
    me->arena = arena;
    return me;
}
