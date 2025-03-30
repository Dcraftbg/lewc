#define TYPE_TABLE_DEFINE
#define MEMBERS_DEFINE
#include "type.h"
Type type_bool   = { .name="bool", .core=CORE_BOOL, .ptr_count=0, .unsign=true  }; 

Type type_i8     = { .name="i8"  , .core=CORE_I8  , .ptr_count=0, .unsign=false },
     type_u8     = { .name="u8"  , .core=CORE_I8  , .ptr_count=0, .unsign=true  };

Type type_i16    = { .name="i16" , .core=CORE_I16 , .ptr_count=0, .unsign=false },
     type_u16    = { .name="u16" , .core=CORE_I16 , .ptr_count=0, .unsign=true  };

Type type_i32    = { .name="i32" , .core=CORE_I32 , .ptr_count=0, .unsign=false },
     type_u32    = { .name="u32" , .core=CORE_I32 , .ptr_count=0, .unsign=true  };

Type type_i64    = { .name="i64" , .core=CORE_I64 , .ptr_count=0, .unsign=false },
     type_u64    = { .name="u64" , .core=CORE_I64 , .ptr_count=0, .unsign=true  };

Type type_u8_ptr = { .name=NULL  , .core=CORE_PTR , .ptr_count=1, .inner_type=&type_u8};
Type* core_types[] = {
    &type_bool,
    &type_i8,
    &type_u8,
    &type_i16,
    &type_u16,
    &type_i32,
    &type_u32,
    &type_i64,
    &type_u64,
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
    case CORE_I64:
    case CORE_PTR:
    case CORE_FUNC:
        return 8;
    case CORE_CONST_ARRAY:
        return type_size(type->array.of) * type->array.len;
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
    case CORE_I64:
    case CORE_FUNC:
    case CORE_PTR:
        return 8;
    case CORE_STRUCT:
        return type->struc.alignment;
    }
    unreachable("type->core=%d", type->core);
}
#include "darray.h"
void struct_add_field(Struct* me, Atom* name, Type* type) {
    size_t align = type_alignment(type);
    if(align > me->alignment) me->alignment = align;
    me->offset += me->offset % align;
    Member member = {
        MEMBER_FIELD,
        type,
        me->offset
    };
    me->offset += type_size(type);
    members_insert(&me->members, name, member);
    da_push(&me->fields, name);
}
Type* type_new(Arena* arena) {
    Type* type = arena_alloc(arena, sizeof(*type));
    assert(type && "Ran out of memory");
    memset(type, 0, sizeof(*type));
    return type;
}
Type* type_new_struct(Arena* arena, const Struct struc) {
    Type* me = type_new(arena);
    me->core = CORE_STRUCT;
    me->struc = struc;
    return me;
}

Type* type_new_const_array(Arena* arena, Type* of, size_t len) {
    Type* me = type_new(arena);
    me->core = CORE_CONST_ARRAY;
    me->array.len = len;
    me->array.of = of;
    return me;
}
Type* type_ptr(Arena* arena, Type* to, size_t ptr_count) {
    assert(to);
    Type* res = type_new(arena);
    res->core = CORE_PTR;
    res->ptr_count = ptr_count;
    if(to->core == CORE_PTR) {
        res->ptr_count += to->ptr_count;
        res->inner_type = to->inner_type;
        return res;
    }
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
        if(t->signature.variadic) {
            if(t->signature.input.len > 0) fprintf(f, ", ");
            assert(t->signature.variadic == VARIADIC_C);
            fprintf(f, "... #c");
        }
        fputc(')', f);
        if(t->signature.output) {
            fprintf(f, " -> ");
            type_dump(f, t->signature.output);
        }
    } break;
    case CORE_CONST_ARRAY:
        fprintf(f, "[");
        type_dump(f, t->array.of);
        fprintf(f, "; %zu]", t->array.len);
        break;
    case CORE_STRUCT: {
        fputs("struct {", f);
        Struct* s = &t->struc;
        for(size_t i = 0; i < s->fields.len; ++i) {
            if(i > 0) fputs(", ", f);
            fprintf(f, "%s : ", s->fields.items[i]->data); type_dump(f, members_get(&s->members, s->fields.items[i])->type); 
        }
        fputs("}", f);
    } break;
    }
}
bool type_isbinary(Type* t) {
    return t && (t->core == CORE_I8 || t->core == CORE_I16 || t->core == CORE_I32 || t->core == CORE_I64 || t->core == CORE_PTR);
}
bool type_isint(Type* t) {
    return t && (t->core == CORE_I8 || t->core == CORE_I16 || t->core == CORE_I32 || t->core == CORE_I64);
}
