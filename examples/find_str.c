#include <stdio.h>
char* find_unique(const char* str, char c);
int main(void) {
    const char* str = "ooBar";
    printf("str=\"%s\"\n", str);
    char c = 'o';
    printf("find_unique(%c) => \"%s\"\n", c, find_unique(str, c));
    return 0;
}
