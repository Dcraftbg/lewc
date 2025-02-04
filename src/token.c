#include "token.h"
#include "strutils.h"
#define TPRINTF(...) snprintf(tdisplay_buf, sizeof(tdisplay_buf), __VA_ARGS__)
const char* tdisplay(Token t) {
    static char tdisplay_buf[1024];
    static_assert(TOKEN_COUNT == 275, "Update tdisplay");
    assert(sizeof(tdisplay_buf) > (t.src_len + 1));
    strncpy(tdisplay_buf, t.src, t.src_len);
    return tdisplay_buf;
}

const char* tloc(Token t) {
    static char tdisplay_buf[1024];
    TPRINTF("%s:%zu:%zu",t.path,t.l0,t.c0);
    return tdisplay_buf;
}
