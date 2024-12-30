#define TYPE_TABLE_DEFINE
#include "type.h"
Type type_u8  = { .core=CORE_I8 , .ptr_count=0, .unsign=true  };
Type type_i32 = { .core=CORE_I32, .ptr_count=0, .unsign=false };

Type* type_new(Arena* arena) {
    Type* type = arena_alloc(arena, sizeof(*type));
    memset(type, 0, sizeof(*type));
    return type;
}
void type_table_init(TypeTable* t) {
    memset(t, 0, sizeof(*t));
    type_table_insert(t, "u8" , &type_u8 );
    type_table_insert(t, "i32", &type_i32);
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
