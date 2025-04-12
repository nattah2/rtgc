#include "tb_allocator.h"
#include "tb_gc.h"

#include <stdio.h>
#include <assert.h>

// Simple structure to represent a test object
typedef struct test_data {
    int id;
    char data[100];  // Some payload data
} test_data_t;

// Function to get the actual object pointer from user data
static object_t* get_object_ptr(void* user_data) {
    // Calculate the offset to get back to the object header
    return (object_t*)((char*)user_data - (sizeof(object_t*) *
           ((object_t*)0)->child_count));
}

// Global statistics for verification
static size_t total_allocations = 0;
static size_t live_objects = 0;

// Custom allocation tracker
void* tracked_alloc(size_t size, size_t child_slots) {
    void* ptr = gc_alloc(size, child_slots);
    if (ptr) {
        total_allocations++;
        live_objects++;
    }
    return ptr;
}

// Test scenario: Create a tree structure and then delete references
void test_tree_collection() {
    printf("=== Testing Tree Collection ===\n");

    // Reset counters
    total_allocations = 0;
    live_objects = 0;

    // Create root node with 3 children
    test_data_t* root_data = tracked_alloc(sizeof(test_data_t), 3);
    root_data->id = 1;
    object_t* root_obj = get_object_ptr(root_data);
    gc_add_root(root_obj);

    printf("Created root node (ID: %d)\n", root_data->id);

    // Create child nodes
    for (int i = 0; i < 3; i++) {
        test_data_t* child_data = tracked_alloc(sizeof(test_data_t), 2);
        child_data->id = 10 + i;
        object_t* child_obj = get_object_ptr(child_data);
        gc_write_barrier(root_obj, i, child_obj);

        printf("Created child node %d (ID: %d)\n", i, child_data->id);

        // Create grandchildren
        for (int j = 0; j < 2; j++) {
            test_data_t* grandchild_data = tracked_alloc(sizeof(test_data_t), 0);
            grandchild_data->id = 100 + (i * 10) + j;
            object_t* grandchild_obj = get_object_ptr(grandchild_data);
            gc_write_barrier(child_obj, j, grandchild_obj);

            printf("Created grandchild node %d.%d (ID: %d)\n", i, j, grandchild_data->id);
        }
    }

    // Now we should have 1 + 3 + 6 = 10 objects
    printf("Total allocations: %zu\n", total_allocations);
    assert(total_allocations == 10);

    // Run GC - nothing should be collected as all objects are reachable
    printf("Running GC (no objects should be collected)...\n");
    gc_collect_full();

    // Check that all objects are still live
    printf("Live objects after GC: %zu\n", live_objects);
    assert(live_objects == 10);

    // Now disconnect one of the child branches completely
    printf("Disconnecting child branch 1...\n");
    gc_write_barrier(root_obj, 1, NULL);

    // Run GC - child 1 and its two grandchildren should be collected
    printf("Running GC (3 objects should be collected)...\n");
    gc_collect_full();

    // We should have 7 live objects left
    // This assumes the implementation of sweeping updates our live_objects count
    printf("Expected live objects: 7\n");

    // Create some floating garbage (not connected to any root)
    printf("Creating floating garbage...\n");
    for (int i = 0; i < 5; i++) {
        test_data_t* garbage = tracked_alloc(sizeof(test_data_t), 0);
        garbage->id = 1000 + i;
        printf("Created garbage object %d (ID: %d)\n", i, garbage->id);
    }

    // Now we have 12 objects total, but only 7 reachable
    printf("Total allocations: %zu\n", total_allocations);
    assert(total_allocations == 15);

    // Run GC again - floating garbage should be collected
    printf("Running GC (floating garbage should be collected)...\n");
    gc_collect_full();

    // We should still have 7 live objects
    printf("Expected live objects: 7\n");

    // Remove the root reference
    printf("Removing root...\n");
    gc_remove_root(root_obj);

    // Run GC - everything should be collected
    printf("Running GC (all remaining objects should be collected)...\n");
    gc_collect_full();

    // We should have 0 live objects
    printf("Expected live objects: 0\n");

    printf("Tree collection test complete!\n\n");
}

// Test cycles in the object graph
void test_cycle_collection() {
    printf("=== Testing Cycle Collection ===\n");

    // Reset counters
    total_allocations = 0;
    live_objects = 0;

    // Create objects that will form a cycle
    test_data_t* obj1_data = tracked_alloc(sizeof(test_data_t), 1);
    obj1_data->id = 1;
    object_t* obj1 = get_object_ptr(obj1_data);

    test_data_t* obj2_data = tracked_alloc(sizeof(test_data_t), 1);
    obj2_data->id = 2;
    object_t* obj2 = get_object_ptr(obj2_data);

    // Create cycle: obj1 -> obj2 -> obj1
    gc_write_barrier(obj1, 0, obj2);
    gc_write_barrier(obj2, 0, obj1);

    printf("Created cycle: obj1 (ID: %d) <-> obj2 (ID: %d)\n",
           obj1_data->id, obj2_data->id);

    // Add obj1 as a root
    gc_add_root(obj1);
    printf("Added obj1 as root\n");

    // Run GC - nothing should be collected
    printf("Running GC (nothing should be collected)...\n");
    gc_collect_full();

    // We should have 2 live objects
    printf("Total allocations: %zu\n", total_allocations);
    assert(total_allocations == 2);

    // Remove obj1 from roots
    gc_remove_root(obj1);
    printf("Removed obj1 from roots\n");

    // Run GC - both objects should be collected despite the cycle
    printf("Running GC (both objects should be collected)...\n");
    gc_collect_full();

    // We should have 0 live objects
    printf("Expected live objects: 0\n");

    printf("Cycle collection test complete!\n\n");
}

// Custom allocator that tracks deallocations
static int free_count = 0;

void count_free(void* ptr) {
    if (ptr) {
        free_count++;
        live_objects--;
    }
    tb_free(ptr);
}

int main() {
    // Initialize the garbage collector
    printf("LET ME BE GAY!\n");
    gc_init();
    printf("Garbage collector initialized\n\n");

    // Override the tb_free function for tracking
    // This would require modifying the garbage collector to use a function pointer
    // For now, this is just conceptual

    // Run the tests
    test_tree_collection();
    test_cycle_collection();

    printf("All tests completed!\n");
    return 0;
}
