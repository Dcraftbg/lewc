#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "utils.h"
// TODO: A way to dump a type
enum {
    CORE_PTR,
    CORE_BOOL,
    CORE_I8,
    CORE_I16,
    CORE_I32,
    CORE_FUNC,
    CORE_STRUCT,
};
enum {
    TYPE_ATTRIB_EXTERN=BIT(1),
};
typedef struct Type Type;
typedef struct {
    enum {
        MEMBER_FIELD,
        MEMBER_COUNT
    } kind;
    Type* type;
    size_t offset;
} Member;
#include "func.h"
#ifdef MEMBERS_DEFINE
#    define HASHMAP_DEFINE
#endif
#include "hashmap.h"
#define MEMBERS_ALLOC(n) malloc(n)
#define MEMBERS_DEALLOC(ptr, size) free(ptr)
MAKE_HASHMAP_EX(Members, members, Member, Atom*, atom_hash, atom_eq, MEMBERS_ALLOC, MEMBERS_DEALLOC);
#ifdef MEMBERS_DEFINE
#    undef  HASHMAP_DEFINE
#endif
// TODO: packed flag for packed structures
typedef struct {
    Members members;
    struct {
        Atom **items;
        size_t len, cap;
    } fields;
    size_t offset, alignment;
} Struct;
void struct_add_field(Struct* struc, Atom* name, Type* type);
// TODO: const char* name into Atom*
// FIXME: Memory leak with Struct. Its fine but maybe clean it up if it becomes a problem
struct Type {
    const char* name;
    int core;
    size_t ptr_count;
    // TODO: Move the extern thing to func.h
    uint8_t attribs;
    union {
       bool unsign; // Is it unsigned?
       FuncSignature signature;
       Type* inner_type;
       Struct struc;
    };
};

size_t type_size(Type* type);
size_t type_alignment(Type* type);
// TODO: type_eq for structs fields and whatnot
static bool type_eq(Type* a, Type* b) {
    if(a == NULL && b == NULL) return true;
    if(a == NULL || b == NULL) return false;
    if(a->core != b->core || a->ptr_count != b->ptr_count) return false;
    if(a->core == CORE_PTR) return type_eq(a->inner_type, b->inner_type);
    return true;
}
#ifdef TYPE_TABLE_DEFINE
#   define HASHMAP_DEFINE
#endif
#include "hashmap.h"
#define TYPE_TABLE_ALLOC(n) malloc(n)
#define TYPE_TABLE_DEALLOC(ptr, size) free(ptr)
#define cstr_eq(a, b) (strcmp(a, b) == 0)
MAKE_HASHMAP_EX(TypeTable, type_table, Type*, const char*, djb2_cstr, cstr_eq, TYPE_TABLE_ALLOC, TYPE_TABLE_DEALLOC);
#ifdef TYPE_TABLE_DEFINE
#   undef HASHMAP_DEFINE
#endif
extern Type type_bool;
extern Type type_u8;
extern Type type_u8_ptr;
extern Type type_u16;
extern Type type_i32;
Type* type_ptr(Arena* arena, Type* to, size_t ptr_count);
void type_table_move(TypeTable* into, TypeTable* from);
void type_table_init(TypeTable* t);
#include "arena.h"
Type* type_new(Arena* arena);
Type* type_new_struct(Arena* arena, const Struct struc);
bool type_isbinary(Type* t);
void type_dump(FILE* f, Type* t);
