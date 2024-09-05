#include "strutils.h"
void strescape(const char* str, size_t len, ScratchBuf* buf) {
    for(size_t i = 0; i < len; ++i) {
        char c = str[i];
        switch(c) {
        case '\n':
            scratchbuf_push(buf, '\\');
            scratchbuf_push(buf, 'n');
            break;
        case '\t':
            scratchbuf_push(buf, '\\');
            scratchbuf_push(buf, 't');
            break;
        case '\r':
            scratchbuf_push(buf, '\\');
            scratchbuf_push(buf, 'r');
            break;
        // NOTE: not really reachable so I dunno why its even here
        case '\0':
            scratchbuf_push(buf, '\\');
            scratchbuf_push(buf, '0');
            break;
        default:
            scratchbuf_push(buf, c);
            break;
        }
    }
    scratchbuf_push(buf, '\0');
}
