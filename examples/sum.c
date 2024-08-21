#include <stdio.h>
int sum(int a, int b);
int main(void) {
    int a = 4;
    int b = 5;
    printf("sum(%d,%d) => %d\n",a,b,sum(a,b));
}
