#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include "tb_gc.h"

// Helper function to recover the object header from the user pointer

typedef struct object {
    size_t size;
    uint8_t marked;
    struct object **children;
    size_t child_count;
    struct object *next_object;
} object_t;

/* extern object_t *all_objects; */

object_t *extract_object(void *user_ptr, size_t child_count) {
    return (object_t *)((uint8_t *)user_ptr - sizeof(object_t) - sizeof(object_t *) * child_count);
}

void test_object_graph_gc() {
    printf("=== Test: Object Graph GC ===\n");

    gc_init();

    void *a_data = gc_alloc(16, 1);
    void *b_data = gc_alloc(16, 1);
    void *c_data = gc_alloc(16, 1);
    void *d_data = gc_alloc(16, 0);

    object_t *a = extract_object(a_data, 1);
    object_t *b = extract_object(b_data, 1);
    object_t *c = extract_object(c_data, 1);
    object_t *d = extract_object(d_data, 0);

    // Connect the graph
    gc_write_barrier(a, 0, b);
    gc_write_barrier(b, 0, c);
    gc_write_barrier(c, 0, d);

    // Add root: a
    gc_add_root(a);

    gc_collect_full();

    // All should be alive
    printf("[After first GC — everything reachable from a]\n");
    printf("a alive? %s\n", findObj(a) ? "yes" : "no");
    printf("b alive? %s\n", findObj(b) ? "yes" : "no");
    printf("c alive? %s\n", findObj(c) ? "yes" : "no");
    printf("d alive? %s\n", findObj(d) ? "yes" : "no");

    // Cut connection to b (and rest of graph)
    gc_write_barrier(a, 0, NULL);

    gc_collect_full();

    // Now b, c, d should be collected
    printf("[After second GC — a is root but no children]\n");
    printf("a alive? %s\n", findObj(a) ? "yes" : "no");
    printf("b alive? %s\n", findObj(b) ? "yes" : "no (expected)");
    printf("c alive? %s\n", findObj(c) ? "yes" : "no (expected)");
    printf("d alive? %s\n", findObj(d) ? "yes" : "no (expected)");
}

int main() {
    test_object_graph_gc();
    return 0;
}
