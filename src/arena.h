#pragma once
#include <stddef.h>
typedef struct ArenaBlock {
    struct ArenaBlock* next;
    size_t head;
    size_t cap;
    char data[];
} ArenaBlock;
typedef struct {
    ArenaBlock* first;
} Arena;
// TODO: error message on missing ARENA_MALLOC definitions?
#define INIT_ARENA_SIZE 4096
#if !defined(ARENA_MALLOC) && !defined(ARENA_FREE)
#   include <stdlib.h>
#   define ARENA_MALLOC(x) malloc(x)
#   define ARENA_FREE(x) free(x)
#endif
ArenaBlock* new_arena_block(size_t cap);
void* arena_alloc(Arena* arena, size_t size);
#include <stdarg.h>
const char* vaprintf(Arena* arena, const char* fmt, va_list args);

const char* aprintf(Arena* arena, const char* fmt, ...) __attribute__((format(printf,2,3)));

