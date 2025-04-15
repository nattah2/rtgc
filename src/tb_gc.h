#ifndef TB_GC_H
#define TB_GC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declare object type
typedef struct object object_t;

/**
 * Initializes the garbage collector and allocator.
 */
void gc_init(void);

/**
 * Allocates memory managed by the GC.
 *
 * @param size         Number of bytes for user data.
 * @param child_slots  Number of child references the object can hold.
 * @return             Pointer to user data area.
 */
void *gc_alloc(size_t size, size_t child_slots);

/**
 * Adds an object to the root set.
 *
 * @param obj Pointer to an object.
 */
void gc_add_root(object_t *obj);

/**
 * Removes an object from the root set.
 *
 * @param obj Pointer to an object.
 */
void gc_remove_root(object_t *obj);
int findObj(object_t *obj);

/**
 * Performs a full garbage collection (mark and sweep).
 */
void gc_collect_full(void);

/**
 * Performs one step of incremental garbage collection.
 */
void gc_collect_step(void);

/**
 * Write barrier for pointer updates to maintain correctness during GC.
 *
 * @param parent The parent object being updated.
 * @param slot   The child slot being updated.
 * @param child  The new child object.
 */
void gc_write_barrier(object_t *parent, size_t slot, object_t *child);


#ifdef __cplusplus
}
#endif

#endif // TB_GC_H
