#include <stdio.h>
#include <string.h>
#include "tb_allocator.h"

int main() {
    tb_initialize_allocator();

    void *a = tb_malloc(100);
    void *b = tb_malloc(200);
    void *c = tb_malloc(50);

    printf("Allocated a: %p\n", a);
    printf("Allocated b: %p\n", b);
    printf("Allocated c: %p\n", c);

    strcpy(a, "Hello");
    strcpy(b, "World");
    strcpy(c, "!");

    printf("%s %s%s\n", (char*)a, (char*)b, (char*)c);

    tb_free(b);
    tb_free(a);
    tb_free(c);

    void *d = tb_malloc(300);
    printf("Allocated d after free: %p\n", d);

    tb_free(d);
    return 0;
}



