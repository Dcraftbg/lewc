#include "scratch.h"
void scratchbuf_reserve(ScratchBuf* buf, size_t extra) {
    size_t nlen = buf->len + extra;
    if (nlen > buf->cap) {
         size_t ncap = buf->cap * 2 + nlen;
         eprintfln("[scratchbuf_reserve] buf->data = %p buf->_inline = %p\n", buf->data, buf->_inline);
         if(buf->data == buf->_inline) {
             assert(buf->cap == SCRATCHBUF_INLINE);
             buf->data = malloc(ncap);
             assert(buf->data && "Ran out of memory");
             memcpy(buf->data, buf->_inline, buf->len);
             buf->cap = ncap;
         } else {
             void* op = buf->data;
             eprintfln("realloc on data=%p _inline=%p and cap=%zu\n", buf->data, buf->_inline, buf->cap);
             buf->data = realloc(buf->data, ncap);
             if(!buf->data) {
                 free(op);
                 assert(false && "Ran out of memory");
             }
             buf->cap = ncap;
         }
    }
}
