#include "token.h"
#include "strutils.h"
#define TPRINTF(...) snprintf(tdisplay_buf, sizeof(tdisplay_buf), __VA_ARGS__)
const char* tdisplay(Token t) {
    static char tdisplay_buf[1024];
    switch(t.kind) {
    case TOKEN_ATOM:
        TPRINTF("Word(%s)",t.atom->data);
        return tdisplay_buf;
    case TOKEN_EOF:
        TPRINTF("EOF");
        return tdisplay_buf;
    case TOKEN_UNPARSABLE:
        if(t.codepoint <= 127) {
           TPRINTF("UNPARSABLE (%c)",(char)t.codepoint);
        } else {
           TPRINTF("UNPARSABLE (%08X)",t.codepoint);
        }
        return tdisplay_buf;
    case TOKEN_ARROW:
        TPRINTF("->");
        return tdisplay_buf;
    case TOKEN_RETURN:
        TPRINTF("return");
        return tdisplay_buf;
    case TOKEN_C_STR: {
        ScratchBuf buf;
        scratchbuf_init(&buf);
        strescape(t.str, t.str_len, &buf);
        TPRINTF("CStr (\"%s\")", buf.data);
        scratchbuf_cleanup(&buf);
        return tdisplay_buf;
    }
    default:
        if(t.kind < 256) {
           TPRINTF("'%c'",t.kind);
           return tdisplay_buf;
        }
    }
    eprintfln("None exhaustive tdisplay for token.kind = %d",t.kind);
    abort();
    return NULL;
}

const char* tloc(Token t) {
    static char tdisplay_buf[1024];
    TPRINTF("%s:%zu:%zu",t.path,t.l0,t.c0);
    return tdisplay_buf;
}
