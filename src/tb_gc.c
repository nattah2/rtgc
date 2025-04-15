#ifndef TB_GC_H
#define TB_GC_H

#define MAX_ROOTS 1024
#define MARK_STACK_SIZE 1024
#include "tb_allocator.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

typedef struct object {
    size_t size;
    uint8_t marked;
    struct object **children;
    size_t child_count;
    struct object *next_object; // For tracking all allocated objects
} object_t;

/* ========================= GARBAGE COLLECTOR DATA STRUCTURES ========================= */

static object_t *root_set[MAX_ROOTS];
static size_t root_count = 0;
static object_t *mark_stack[MARK_STACK_SIZE];
static size_t mark_top = 0;
static object_t *all_objects = NULL;  // Linked list of all allocated objects
static pthread_mutex_t gc_lock = PTHREAD_MUTEX_INITIALIZER;
static int gc_collection_in_progress = 0;

/* ========================= GARBAGE COLLECTOR FUNCTIONS ========================= */

void gc_init(void) {
    tb_initialize_allocator();
    root_count = 0;
    mark_top = 0;
    all_objects = NULL;
}

void gc_add_root(object_t *obj) {
    if (!obj) return;

    pthread_mutex_lock(&gc_lock);
    if (root_count < MAX_ROOTS) {
        root_set[root_count++] = obj;
    }
    pthread_mutex_unlock(&gc_lock);
    //printf("[DEBUG] Added root %p. Total roots: %zu\n", obj, root_count);
}

void gc_remove_root(object_t *obj) {
    if (!obj) return;

    pthread_mutex_lock(&gc_lock);
    for (size_t i = 0; i < root_count; i++) {
        if (root_set[i] == obj) {
            // Move the last root to this position and decrement count
            root_set[i] = root_set[--root_count];
            break;
        }
    }
    pthread_mutex_unlock(&gc_lock);
}

void *gc_alloc(size_t size, size_t child_slots) {
    // Calculate total size needed
    size_t object_size = sizeof(object_t) + sizeof(object_t *) * child_slots + size;

    // Allocate memory using the buddy allocator
    object_t *obj = tb_malloc(object_size);
    if (!obj) return NULL;

    // Initialize object fields
    obj->size = size;
    obj->marked = 0;
    obj->child_count = child_slots;
    obj->children = (object_t **)(obj + 1);

    // Clear the children array
    memset(obj->children, 0, sizeof(object_t *) * child_slots);

    // Add to global object list for tracking
    pthread_mutex_lock(&gc_lock);
    obj->next_object = all_objects;
    all_objects = obj;
    pthread_mutex_unlock(&gc_lock);

    // Return pointer to user data area (after header and children array)
    return (void*)(obj->children + child_slots);
}

static int is_tracked_object(object_t *obj) {
    object_t *curr = all_objects;
    while (curr) {
        if (curr == obj) return 1;
        curr = curr->next_object;
    }
    return 0;
}

static void push_mark_stack(object_t *obj) {
    if (mark_top < MARK_STACK_SIZE) {
        mark_stack[mark_top++] = obj;
    } else {
        // Mark stack overflow - process some objects immediately
        for (size_t i = 0; i < mark_top / 2; i++) {
            object_t *to_process = mark_stack[i];
            for (size_t j = 0; j < to_process->child_count; j++) {
                if (to_process->children[j] && !to_process->children[j]->marked) {
                    to_process->children[j]->marked = 1;
                }
            }
        }

        // Compress stack
        mark_top /= 2;
        mark_stack[mark_top++] = obj;
    }
}

static object_t *pop_mark_stack(void) {
    return (mark_top == 0) ? NULL : mark_stack[--mark_top];
}

static void mark_object(object_t *obj) {
    if (!obj || obj->marked || !is_tracked_object(obj)) {
        return;
    };

    obj->marked = 1;
    push_mark_stack(obj);
}

static void mark_phase(void) {
    pthread_mutex_lock(&gc_lock);
    gc_collection_in_progress = 1;

    // Mark from all roots
    for (size_t i = 0; i < root_count; i++) {
        mark_object(root_set[i]);
    }

    // Process mark stack
    while (mark_top > 0) {
        object_t *obj = pop_mark_stack();
        for (size_t i = 0; i < obj->child_count; i++) {
            mark_object(obj->children[i]);
        }
    }

    pthread_mutex_unlock(&gc_lock);
}

static void sweep_phase(void) {
    pthread_mutex_lock(&gc_lock);

    object_t **prev = &all_objects;
    object_t *curr = all_objects;

    // Traverse all objects
    while (curr) {
        if (!curr->marked) {
            // Not marked - remove from list and free
            *prev = curr->next_object;
            object_t *to_free = curr;
            curr = curr->next_object;
            tb_free(to_free);
        } else {
            // Marked - reset mark and move to next
            curr->marked = 0;
            prev = &curr->next_object;
            curr = curr->next_object;
        }
    }

    gc_collection_in_progress = 0;
    pthread_mutex_unlock(&gc_lock);
}

void gc_collect_step(void) {
    // One incremental step of GC (simplified for this example)
    mark_phase();
    sweep_phase();
}

void gc_collect_full(void) {
    printf("=== Starting GC ===\n");

    mark_phase();

    // Print marking results BEFORE sweep
    /* printf("Marked objects:\n"); */
    object_t *obj = all_objects;
    while (obj) {
        /* printf("  Object %p: marked=%d\n", (void*)obj, obj->marked); */
        obj = obj->next_object;
    }

    sweep_phase();
}

// Write barrier for incremental collection
void gc_write_barrier(object_t *parent, size_t slot, object_t *child) {
    if (!parent || slot >= parent->child_count) return;

    parent->children[slot] = child;

    // If collection is in progress and parent is marked but child is not,
    // mark the child to maintain correctness
    if (gc_collection_in_progress && parent->marked && child && !child->marked) {
        pthread_mutex_lock(&gc_lock);
        printf("-> parent marked? %d, child marked? %d\n", parent->marked, child ? child->marked : -1);
        mark_object(child);
        pthread_mutex_unlock(&gc_lock);
    }
}

int findObj(void *ptr) {
    object_t *curr = all_objects;
    while (curr) {
        if (curr == ptr) return 1;
        curr = curr->next_object;
    }
    return 0;
}


#endif // TB_GC
