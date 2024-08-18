#pragma once
#include "hashutils.h"
#include "arena.h"
#include <stddef.h>
#include <string.h>
typedef struct {
    size_t len;
    char data[];
} Atom;

#ifdef SMART_ATOM
typedef struct AtomHead {
    struct list list;
    Atom atom;
} AtomHead;
typedef struct {
    struct list list;
    size_t size;
} AtomBucket;
#endif
// TODO: Reimplement Googles' hash table
typedef struct {
    Arena *arena;
#ifdef SMART_ATOM
    AtomBucket* data;
    // TODO: Maybe use integer scaling with powers of 2?
    size_t len; 
    size_t maxbucket; // The longest bucket in the hashmap
    size_t limit;
    size_t scalar;
#error SMART_ATOM is unfinished... Yeahhh.. Im lazy
#endif
} AtomTable;

Atom* atom_alloc(AtomTable* table, const char* str, size_t len);
void atom_table_construct(AtomTable* table);
