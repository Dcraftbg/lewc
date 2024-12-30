#define ATOM_HASHTABLE_DEFINE
#include "atom.h"
Atom* atom_alloc(AtomTable* table, const char* str, size_t len) {
    Atom** entry;
    if((entry=atom_hashmap_get(&table->hashmap, (StrSlice){.data=str, .len=len}))) {
        return *entry;
    }
    Atom* atom = arena_alloc(table->arena, sizeof(Atom) + len + 1);
    if(!atom) return atom;
    atom->len = len;
    memcpy(atom->data, str, len); 
    atom->data[len] = '\0'; // Null terminator
    atom_hashmap_insert(&table->hashmap, (StrSlice){.data=str, .len=len}, atom);
    return atom;
}

void atom_table_construct(AtomTable* table) {
    memset(table, 0, sizeof(*table));
}
