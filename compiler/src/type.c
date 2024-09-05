#include "type.h"
Type* type_table_get(TypeTable* t, size_t id) {
    if(id >= t->len) return NULL;
    return &t->items[id];
}
size_t type_table_create(TypeTable* t, Type type) {
    da_reserve(t, 1);
    size_t id = t->len++;
    t->items[id] = type;
    return id;
}

size_t type_table_get_by_name(TypeTable* t, const char* name) {
    for(size_t i = 0; i < t->len; ++i) {
        if(t->items[i].name && strcmp(t->items[i].name, name) == 0) return i;
    }
    return INVALID_TYPEID;
}

Type builtin_types[] = {
    [BUILTIN_U8]  = { .name="u8" , .core=CORE_I8 , .ptr_count=0, .unsign=true  },
    [BUILTIN_I32] = { .name="i32", .core=CORE_I32, .ptr_count=0, .unsign=false },
};
static_assert(BUILTIN_COUNT==ARRAY_LEN(builtin_types), "Update builtin_types");
void type_table_init(TypeTable* t) {
    memset(t, 0, sizeof(*t));
    for(size_t i = 0; i < ARRAY_LEN(builtin_types); ++i) {
        type_table_create(t, builtin_types[i]);
    } 
}


void type_table_move(TypeTable* into, TypeTable* from) {
    *into = *from;
    memset(from, 0, sizeof(*from));
}

size_t type_get_size(Type* type) {
    switch(type->core) {
    case CORE_I32:
        return 4;
    case CORE_I8:
        return 1;
    default:
        return 0;
    }
}
