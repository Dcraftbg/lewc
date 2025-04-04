#include "location.h"
#include <stdio.h>
// TODO: Just define temp.h an Arena
// where you can just dump this shit and use aprintf
#define TPRINTF(...) snprintf(tdisplay_buf, sizeof(tdisplay_buf), __VA_ARGS__)
const char* tloc(const Location* loc) {
    static char tdisplay_buf[1024];
    TPRINTF("%s:%zu:%zu", loc->path, loc->l0, loc->c0);
    return tdisplay_buf;
}

Location loc_join(const Location* a, const Location* b) {
    return (Location) {
        .path = a->path,
        .l0 = a->l0,
        .c0 = a->c0,
        .l1 = b->l1,
        .c1 = b->c1,
        .src = a->src,
        .src_len = (b->src + b->src_len) - a->src,
    };
}
