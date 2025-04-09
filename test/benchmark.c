#include "benchmark.h"
#include "tb_allocator.h"

void benchmark_tb_malloc() {
    clock_t start = clock();
    void* ptrs[OBJECT_COUNT];

    for (int i = 0; i < OBJECT_COUNT; i++) {
        ptrs[i] = tb_malloc(16);
    }

    clock_t end = clock();
    printf("tb_malloc() time: %lf ms\n", ((double)(end - start) / CLOCKS_PER_SEC) * 1000);

    for (int i = 0; i < OBJECT_COUNT; i++) {
        tb_free(ptrs[i]);
    }
}
