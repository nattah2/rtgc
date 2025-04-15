#include "tb_allocator.h"
#include <stdio.h>
#include <assert.h>

void test_basic_allocation() {
    printf("=== Testing basic allocation ===\n");
    int *a = (int*)tb_malloc(sizeof(int));
    *a = 42;
    printf("Allocated int: %d\n", *a);
    tb_free(a);
    printf("Freed int.\n");
}

void test_large_allocation() {
    printf("\n=== Testing large allocation ===\n");
    size_t large_size = MAX_BLOCK_SIZE + 1;
    char *large = (char*)tb_malloc(large_size);
    assert(large != NULL);
    printf("Allocated large block (%zu bytes).\n", large_size);
    memset(large, 'A', large_size);
    printf("Filled large block with 'A's.\n");
    tb_free(large);
    printf("Freed large block.\n");
}

void test_thread_safety() {
    printf("\n=== Testing thread safety ===\n");
    #pragma omp parallel for
    for (int i = 0; i < 10; i++) {
        int *ptr = (int*)tb_malloc(sizeof(int));
        *ptr = i;
        printf("Thread %d: %d\n", i, *ptr);
        tb_free(ptr);
    }
}

int main() {
    tb_initialize_allocator();

    test_basic_allocation();
    test_large_allocation();
    test_thread_safety();

    tb_cleanup_allocator();
    return 0;
}
