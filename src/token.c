#include "token.h"
#include "strutils.h"
#define TPRINTF(...) snprintf(tdisplay_buf, sizeof(tdisplay_buf), __VA_ARGS__)
const char* tdisplay(Token t) {
    static_assert(TOKEN_COUNT-TOKEN_ERR == 3, "Update tdisplay errors");
    static char tdisplay_buf[1024];
    switch(t.kind) {
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
    case TOKEN_INVALID_STR:
        TPRINTF("Invalid String");
        return tdisplay_buf;
    case TOKEN_INVALID_INT_LITERAL:
        TPRINTF("Invalid integer literal");
        return tdisplay_buf;
    default:
        assert(sizeof(tdisplay_buf) > (t.loc.src_len + 1));
        strncpy(tdisplay_buf, t.loc.src, t.loc.src_len);
        return tdisplay_buf;
    }
    unreachable("Non exhaustive tdisplay");
}

const char* tloc(Token t) {
    static char tdisplay_buf[1024];
    TPRINTF("%s:%zu:%zu",t.loc.path,t.loc.l0,t.loc.c0);
    return tdisplay_buf;
}
