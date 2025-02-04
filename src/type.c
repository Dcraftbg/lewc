#define TYPE_TABLE_DEFINE
#define MEMBERS_DEFINE
#include "type.h"
Type type_bool   = { .name="bool", .core=CORE_BOOL, .ptr_count=0, .unsign=true  }; 
Type type_u8     = { .name="u8"  , .core=CORE_I8  , .ptr_count=0, .unsign=true  };
Type type_u16    = { .name="u16" , .core=CORE_I16 , .ptr_count=0, .unsign=true  };
Type type_i32    = { .name="i32" , .core=CORE_I32 , .ptr_count=0, .unsign=false };
Type type_u8_ptr = { .name=NULL  , .core=CORE_PTR , .ptr_count=1, .inner_type=&type_u8};
Type* core_types[] = {
    &type_bool,
    &type_u8,
    &type_u16,
    &type_i32,
};
// TODO: Maybe have a more fine control over size for smaller types in bits
// FIXME: Stuff like pointer size / function size depend on architecture.
// Future me problem I guess xD
size_t type_size(Type* type) {
    switch(type->core) {
    case CORE_BOOL:
    case CORE_I8:
        return 1;
    case CORE_I16:
        return 2;
    case CORE_I32:
        return 4;
    case CORE_PTR:
    case CORE_FUNC:
        return 8;
    case CORE_STRUCT:
        return type->struc.offset + type->struc.offset % type->struc.alignment;
    }
    unreachable("type->core=%d", type->core);
}
size_t type_alignment(Type* type) {
    switch(type->core) {
    case CORE_I8:
    case CORE_BOOL:
        return 1;
    case CORE_I16:
        return 2;
    case CORE_I32:
        return 4;
    case CORE_FUNC:
    case CORE_PTR:
        return 8;
    case CORE_STRUCT:
        return type->struc.alignment;
    }
    unreachable("type->core=%d", type->core);
}
void struct_add_field(Struct* me, Atom* name, Type* type) {
    size_t align = type_alignment(type);
    if(align > me->alignment) me->alignment = align;
    me->offset += me->offset % align;
    Member member = {
        MEMBER_FIELD,
        type,
        me->offset
    };
    members_insert(&me->members, name, member);
}
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
    for(size_t i = 0; i < sizeof(core_types)/sizeof(*core_types); ++i) {
        type_table_insert(t, core_types[i]->name, core_types[i]);
    }
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
    if(t->name) {
        fputs(t->name, f);
        return;
    }
    for(size_t i = 0; i < t->ptr_count; ++i) {
        fputc('*', f);
    }
    switch(t->core) {
    case CORE_PTR:
        type_dump(f, t->inner_type);
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
bool type_isbinary(Type* t) {
    if(t->core != CORE_I8 && t->core != CORE_I16 && t->core != CORE_I32 && t->core != CORE_PTR) return false;
    return true;
}
