#include "arena.h"

ArenaBlock* new_arena_block(size_t cap) {
    ArenaBlock* block = (ArenaBlock*)ARENA_MALLOC(sizeof(*block) + cap);
    block->next = NULL;
    block->head = 0;
    block->cap = cap;
    return block;
}
void* arena_alloc(Arena* arena, size_t size) {
    if(!arena->first) {
        arena->first = new_arena_block(INIT_ARENA_SIZE < size ? size : INIT_ARENA_SIZE);
        void* at = arena->first->data + arena->first->head;
        arena->first->head+=size;
        return at;
    }
    ArenaBlock* block = arena->first;
    ArenaBlock* prev = block;
    while(block) {
        if(block->cap-block->head >= size) {
            void* at = block->data + block->head;
            block->head+=size;
            return at;
        }
        prev = block;
        block = block->next;
    }
    prev->next = new_arena_block(INIT_ARENA_SIZE < size ? size : INIT_ARENA_SIZE);
    void* at = prev->next->data + prev->next->head;
    prev->next->head+=size;
    return at;
}

#include <stdio.h>
const char* aprintf(Arena* arena, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
        size_t count = vsnprintf(NULL, 0, fmt, args) + 1;
        char* buf = arena_alloc(arena, count);
    va_end(args);
    va_start(args, fmt);
        vsnprintf(buf, count, fmt, args);
    va_end(args);
    return buf;
}
