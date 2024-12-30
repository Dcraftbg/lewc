#pragma once
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "utils.h"

#define SCRATCHBUF_INLINE 512
typedef struct {
    char* data;
    size_t len;
    size_t cap;
    char _inline[SCRATCHBUF_INLINE];
} ScratchBuf;

static inline void scratchbuf_init(ScratchBuf* buf) {
    buf->data = buf->_inline;
    buf->len = 0;
    buf->cap = SCRATCHBUF_INLINE;
}
static inline void scratchbuf_cleanup(ScratchBuf* buf) {
    if(buf->data != buf->_inline)
        free(buf->data);
}
void scratchbuf_reserve(ScratchBuf* buf, size_t extra); 
static inline void scratchbuf_push(ScratchBuf* buf, char c) {
    scratchbuf_reserve(buf, 1);
    buf->data[buf->len++] = c;
}
static inline void scratchbuf_reset(ScratchBuf* buf) {
    buf->len = 0;
}
