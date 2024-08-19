#include "atom.h"
Atom* atom_alloc(AtomTable* table, const char* str, size_t len) {
    Atom* atom = arena_alloc(table->arena, sizeof(Atom) + len + 1);
    if(!atom) return atom;
    atom->len = len;
    memcpy(atom->data, str, len); 
    atom->data[len] = '\0'; // Null terminator
    return atom;
}

void atom_table_construct(AtomTable* table) {
    memset(table, 0, sizeof(*table));
}




















#ifdef SMART_ATOM
AtomHead* new_atom_head(Arena* arena, const char* str, size_t len) {
    AtomHead* head = arena_alloc(arena, sizeof(*head) + len + 1);
    if(!head) return NULL;
    list_init(&head->list);
    head->atom.len = len;
    memcpy(head->atom.data, str, len); 
    head->atom.data[len] = '\0'; // Null terminator
    return head;
}
#define ATOM_BUCKET_FOREACH(bucket, head) \
    for((head)=(void*)(bucket)->list.next; (head) != (void*)&(bucket)->list; (head)=(void*)(head)->list.next)
#define ATOM_TABLE_FOREACH(table, head) \
    for(size_t _i=0; _i < (table)->len; ++_i) \
        ATOM_BUCKET_FOREACH(&(table)->data[_i], head)
    
void atom_table_rehash_resize(AtomTable* table, size_t extra) {
    size_t nsize = table->len * 2 + extra; // 4/3rds + scalar
    AtomBucket *data = malloc(nsize * sizeof(*table->data));
    if(!data) {
        fprintf(stderr, "RAN OUT OF MEMORY\n");
        abort();
    }
    for(size_t i = 0; i < nsize; ++i) {
        memset(&data[i], 0, sizeof(data[i]));
        list_init(&data[i].list);
    }
    // memset(data, 0, nsize * sizeof(*data));
    if(table->data) {
        AtomHead* head = NULL;

        for(size_t _i=0; _i < (table)->len; ++_i) {
            AtomBucket* oldbucket = &table->data[_i];
            printf("oldbucket: %p\n",oldbucket);
            printf("size: %zu\n",oldbucket->size);
            printf("next: %p\n",oldbucket->list.next);
            printf("prev: %p\n",oldbucket->list.prev);
            AtomHead* next = (AtomHead*)head->list.next;
            for(head=next; (head) != (void*)&oldbucket->list; head=next) {
                printf("next: %p\n",next);
                next = (AtomHead*)head->list.next;
                list_remove(&head->list);
                size_t hash = djb2(head->atom.data, head->atom.len);
                size_t at = hash % nsize;
                AtomBucket* bucket = &data[at]; 
                list_insert(bucket->list.prev, &head->list);
                if(++bucket->size >= table->maxbucket) table->maxbucket = bucket->size;
            }
        }
    }
    table->data = data;
    table->len = nsize;
}
void atom_table_resize(AtomTable* table, size_t extra) {
    if(table->len == 0 || table->maxbucket >= table->limit) {
        atom_table_rehash_resize(table, table->scalar);
    }
}
Atom* atom_alloc(AtomTable* table, const char* str, size_t len) {
    atom_table_resize(table, 1);
    size_t hash = djb2(str, len);
    AtomBucket* bucket = &table->data[hash % table->len];
    AtomHead* head = NULL;
    if(list_empty(&bucket->list)) {
        head = new_atom_head(&table->arena, str, len);
        list_prepend(&bucket->list, &head->list);
        bucket->size++;
        if(table->maxbucket < bucket->size) table->maxbucket = bucket->size;
        return &head->atom;
    }
    ATOM_BUCKET_FOREACH(bucket, head) {
        if(head->atom.len == len && memcmp(head->atom.data, str, len) == 0) return &head->atom;
    }
    head = new_atom_head(&table->arena, str, len);
    list_prepend(&bucket->list, &head->list);
    bucket->size++;
    if(table->maxbucket < bucket->size) table->maxbucket = bucket->size;
    return &head->atom;
    // (AtomHead*)arena_alloc(&table->arena, sizeof(AtomHead)+len+1);
}

void atom_table_construct(AtomTable* table) {
    table->scalar = 10;
    table->maxbucket = 0;
    table->limit = 5;
}
#endif
