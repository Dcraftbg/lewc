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
