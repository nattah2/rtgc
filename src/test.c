#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stddef.h>
#include <pthread.h>
#include <math.h>
#include <stdbool.h>

typedef union alignas(16) header {
    struct {
        size_t size;
        uint32_t is_free;
        header* next;
    } s;
    char stub[16];
} header_t;

int main() {
    printf("%d", sizeof(header_t));
}
