#pragma once
#include "strslice.h"
#include "arena.h"
#include <stddef.h>
#include <string.h>
typedef struct Atom Atom;
struct Atom {
    size_t len;
    char data[];
};

#ifdef ATOM_HASHTABLE_DEFINE
#define HASHMAP_DEFINE
#endif
#include "hashmap.h"
#define ATOMHASHMAP_ALLOC(n) malloc(n)
#define ATOMHASHMAP_DEALLOC(ptr, size) free(ptr)

MAKE_HASHMAP_EX(AtomHashmap, atom_hashmap, Atom*, StrSlice, strslice_hash, strslice_eq, ATOMHASHMAP_ALLOC, ATOMHASHMAP_DEALLOC)
#ifdef ATOM_HASHTABLE_DEFINE
#undef ATOM_HASHTABLE_DEFINE
#endif
typedef struct {
    Arena *arena;
    AtomHashmap hashmap;
} AtomTable;

Atom* atom_alloc(AtomTable* table, const char* str, size_t len);
void atom_table_construct(AtomTable* table);

#define atom_hash(atom) ((size_t)(atom))
#define atom_eq(a, b) ((a)==(b))
