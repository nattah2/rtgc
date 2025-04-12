#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "../src/tb_allocator.h"
#include "../src/tb_gc.h"

#define MAX_ITERATIONS 100000
#define CHILDREN_PER_NODE 10
#define CLEAR_THRESHOLD 10000
#define CSV_FILE "gc_test.csv"

// You can define this depending on your GC structure
typedef struct Node {
    struct Node** children;
} Node;

static inline uint64_t now_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
}

// Fake memory usage tracker for now (optionally improve later)
static size_t current_memory_usage_bytes() {
    // Replace with mallinfo or parse /proc/self/status if desired
    return 0; // placeholder
}

int main(void) {
    FILE* fp = fopen(CSV_FILE, "w");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    fprintf(fp, "Iteration,TaskDelay(us),GCStepTime(us),MemoryUsage(bytes)\n");

    tb_allocator_t* allocator = tb_allocator_create();
    tb_gc_t* gc = tb_gc_create(allocator);

    Node** roots = malloc(sizeof(Node*) * CLEAR_THRESHOLD);
    size_t root_count = 0;

    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        // Allocate node and its children
        Node* node = tb_allocate(gc, sizeof(Node));
        node->children = tb_allocate(gc, sizeof(Node*) * CHILDREN_PER_NODE);
        for (int j = 0; j < CHILDREN_PER_NODE; ++j) {
            node->children[j] = tb_allocate(gc, sizeof(Node));
        }

        roots[root_count++] = node;

        // Simulate real-time task
        uint64_t start = now_us();
        usleep(1000); // 1ms simulated task
        uint64_t end = now_us();
        uint64_t task_delay = end - start;

        // Step GC
        uint64_t gc_start = now_us();
        tb_gc_step(gc);
        uint64_t gc_end = now_us();
        uint64_t gc_time = gc_end - gc_start;

        // Record metrics
        size_t mem = current_memory_usage_bytes(); // zero for now
        fprintf(fp, "%d,%lu,%lu,%zu\n", i, task_delay, gc_time, mem);

        // Optional cleanup cycle
        if (root_count >= CLEAR_THRESHOLD) {
            root_count = 0; // allow GC to collect
        }
    }

    fclose(fp);
    tb_gc_destroy(gc);
    tb_allocator_destroy(allocator);
    free(roots);

    printf("[GC] Test complete. Results saved to %s\n", CSV_FILE);
    return 0;
}
