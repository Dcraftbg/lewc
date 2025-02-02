#include <stdio.h>

void set_to_69(int *val);
int main(void) {
    int a = 5;
    printf("a = %d\n", a);
    set_to_69(&a);
    printf("a = %d\n", a);
    return 0;
}
