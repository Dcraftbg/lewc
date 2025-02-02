#include <stdio.h>

#define BINOPS \
    X(add, +) \
    X(sub, -) \
    X(mul, *) \
    X(div, /) \
    X(mod, %) \
    X(and, &) \
    X(or , |) \
    X(xor, ^)

#define X(name, ...) \
    int name##_lew (int a, int b);
BINOPS
#undef X
#define STRINGIFY(x) # x
int main(void) {
    size_t n = 0, failed = 0;
    int a = 4, b = 5;
    int expected, actual;
#define X(name, op) \
    fprintf(stderr, "[Test %zu] %d %s %d => %d (%s)...", n++, a, STRINGIFY(op), b, name##_lew (a, b), STRINGIFY(name##_lew)); \
    expected = a op b; \
    actual = name##_lew (a, b); \
    if(expected == actual) fprintf(stderr, "OK\n"); \
    else { \
        failed++; \
        fprintf(stderr, "Expected %d got %d\n", expected, actual); \
    }
    BINOPS
    if(failed) {
        fprintf(stderr, "Failed %zu/%zu\n", failed, n);
        return 1;
    }
    return 0;
}
