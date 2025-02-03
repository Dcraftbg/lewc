#define TYPE_TABLE_DEFINE
#include "type.h"
Type type_bool   = { .core=CORE_BOOL, .ptr_count=0, .unsign=true  }; 
Type type_u8     = { .core=CORE_I8  , .ptr_count=0, .unsign=true  };
Type type_u16    = { .core=CORE_I16 , .ptr_count=0, .unsign=true  };
Type type_i32    = { .core=CORE_I32 , .ptr_count=0, .unsign=false };
Type type_u8_ptr = { .core=CORE_PTR , .ptr_count=1, .inner_type=&type_u8};
Type* type_new(Arena* arena) {
    Type* type = arena_alloc(arena, sizeof(*type));
    assert(type && "Ran out of memory");
    memset(type, 0, sizeof(*type));
    return type;
}
Type* type_ptr(Arena* arena, Type* to, size_t ptr_count) {
    Type* res = type_new(arena);
    res->core = CORE_PTR;
    res->ptr_count = ptr_count;
    res->inner_type = to;
    return res;
}
void type_table_init(TypeTable* t) {
    memset(t, 0, sizeof(*t));
    type_table_insert(t, "u8" , &type_u8 );
    type_table_insert(t, "u16", &type_u16);
    type_table_insert(t, "i32", &type_i32);
    type_table_insert(t, "bool", &type_bool);
}
void type_table_move(TypeTable* into, TypeTable* from) {
    *into = *from;
    memset(from, 0, sizeof(*from));
}
void type_dump(FILE* f, Type* t) {
    if(!t) {
        fprintf(f, "void");
        return;
    }
    for(size_t i = 0; i < t->ptr_count; ++i) {
        fputc('*', f);
    }
    switch(t->core) {
    case CORE_PTR:
        type_dump(f, t->inner_type);
        break;
    case CORE_BOOL:
        fprintf(f, "bool");
        break;
    case CORE_I8:
        fprintf(f, "%c8" , t->unsign ? 'u' : 'i');
        break;
    case CORE_I16:
        fprintf(f, "%c16", t->unsign ? 'u' : 'i');
        break;
    case CORE_I32:
        fprintf(f, "%c32", t->unsign ? 'u' : 'i');
        break;
    case CORE_FUNC: {
        fputc('(', f);
        for(size_t i = 0; i < t->signature.input.len; ++i) {
            if(i > 0) fprintf(f, ", ");
            Arg* arg = &t->signature.input.items[i];
            if(arg->name) {
                fprintf(f, "%s: ", arg->name->data);
            }
            type_dump(f, arg->type);
        }
        fputc(')', f);
        if(t->signature.output) {
            fprintf(f, " -> ");
            type_dump(f, t->signature.output);
        }
    } break;
    }
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
bool type_isbinary(Type* t) {
    if(t->core != CORE_I8 && t->core != CORE_I16 && t->core != CORE_I32 && t->core != CORE_PTR) return false;
    return true;
}
