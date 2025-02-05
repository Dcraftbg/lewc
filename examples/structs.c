#include <stdio.h>

typedef struct {
    int a, b;
} Foo;
Foo take_ptr_and_ret_value(Foo* me);
Foo take_value_and_ret_value(Foo me);
int get_a(Foo me);
int get_b(Foo me);
void set_vals_test_copy(Foo me);
Foo foo_set_69(Foo* me);
void set_vals(Foo* me); 
int main(void) {
    Foo foo = { 34, 35 };
    printf("Values:\n");
    printf(".a = %d\n", foo.a);
    printf(".b = %d\n", foo.b);
    printf("take_ptr_and_ret_value:\n");
    foo = take_ptr_and_ret_value(&foo);
    printf(".a = %d\n", foo.a);
    printf(".b = %d\n", foo.b);
    printf("take_value_and_ret_value:\n");
    foo = take_value_and_ret_value(foo);
    printf(".a = %d\n", foo.a);
    printf(".b = %d\n", foo.b);
    printf("get_a = %d\n", get_a(foo));
    printf("get_b = %d\n", get_b(foo));
    printf("set_vals_test_copy:\n");
    set_vals_test_copy(foo);
    printf(".a = %d\n", foo.a);
    printf(".b = %d\n", foo.b);
    printf("set_vals:\n");
    set_vals(&foo);
    printf(".a = %d\n", foo.a);
    printf(".b = %d\n", foo.b);
}
