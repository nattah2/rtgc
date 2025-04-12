#include "tb_gc.h"
#include <stdio.h>
#include <assert.h>

typedef struct {
    int value;
    object_t *child;
} example_data;

void test_basic_gc() {
    printf("=== Testing Basic GC ===\n");

    // Allocate a root object (with explicit cast)
    example_data *root = (example_data *)gc_alloc(sizeof(example_data), 1);
    assert(root != NULL);
    root->value = 42;
    root->child = NULL;

    // Add as root
    gc_add_root((object_t *)root);

    // Allocate a child object (with explicit cast)
    example_data *child = (example_data *)gc_alloc(sizeof(example_data), 0);
    assert(child != NULL);
    child->value = 100;
    child->child = NULL;

    // Link child to root (using write barrier)
    gc_write_barrier((object_t *)root, 0, (object_t *)child);

    printf("Root: %d, Child: %d\n", root->value, child->value);

    // Force GC (should retain both objects)
    gc_collect_full();
    printf("After GC (both objects retained)\n");

    // Remove child reference and collect again (child should be freed)
    gc_write_barrier((object_t *)root, 0, NULL);
    gc_collect_full();
    printf("After GC (child freed)\n");

    // Clean up
    gc_remove_root((object_t *)root);
    gc_collect_full();
}

void test_memory_reuse() {
    printf("\n=== Testing Memory Reuse ===\n");
    void *ptr1 = gc_alloc(128, 0);
    assert(ptr1 != NULL);
    printf("Allocated 128 bytes at %p\n", ptr1);

    gc_collect_full();  // No roots, so ptr1 should be freed

    void *ptr2 = gc_alloc(128, 0);
    assert(ptr2 != NULL);
    printf("Allocated another 128 bytes at %p\n", ptr2);

    // If memory is reused, ptr2 == ptr1
    printf("Memory %s reused\n", ptr1 == ptr2 ? "was" : "was not");
}

int main() {
    gc_init();
    test_basic_gc();
    test_memory_reuse();
    return 0;
}
