#ifndef ALLOCATOR_H_
#define ALLOCATOR_H_

#include <stddef.h>
#include <pthread.h>
#include <math.h>
#include <stdbool.h>

typedef union alignas(16) header {
    struct {
        size_t size;
        bool is_free;
        header_t* next;
    } s;
    char stub[16];
} header_t;

#define HEAP_SIZE (1 << 20) // A 1-MB heap.
#define MIN_BLOCK_SIZE 32
#define MAX_BLOCK_SIZE 1 << 20
#define MAX_LEVELS log2(MAX_BLOCK_SIZE / MIN_BLOCK_SIZE)

static char heap[HEAP_SIZE];
static header_t* free_list[MAX_LEVELS];
/* 0 => 32 Bytes */
/* 1 => 64 Bytes */

extern header_t *head, *tail;
extern pthread_mutex_t lock;

header_t* tb_find_free_block(int level) {
    header_t* ptr = free_lists[level];  // Access the free list for this level
    while (ptr != NULL) {
        if (ptr->s.is_free == 1) {
            return ptr;  // Found a free block
        }
        ptr = ptr->s.next_free;  // Move to next block in the list
    }
    return NULL;  // No free blocks found
}
void* tb_malloc(size_t size) {
    if (size == 0) return NULL;
    size_t total_size = calculate_total_size(size);
    int level = size_to_level(total_size);
    header_t* block = tb_find_free_block(level);

    /* If a block is available, remove it from the free list
     * and return it. */
    if (block) {
        // Remove from free list
        block->s.is_free = 0;
        return (void*)(block + 1);
    }
    /* If no block is available, split a larger block from
     * a higher level and add the resulting buddies. */
    block = split_larger_block(level);
    if (block) {
        block->s.is_free = 0;
        pthread_mutex_unlock(&lock);
    }
    return (void*)(block + 1);
}
void tb_initialize_allocator() {
    void* heap = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (heap == MAP_FAILED) {
        perror("heap init failed - exiting");
        exit(1);
    }
    header_t* block = (header_t*) heap;
    block->size = HEAP_SIZE - HEADER_SIZE;
    block->is_free = 1;
    free_list = block;
}
int size_to_level(size_t size) {
    total_size = (size + HEADER_SIZE + 15) & ~15;
    // This is less portable because floating point math is evil
    // A loop may be preferable.
    /* int level = ceil(log2(size + HEADER_SIZE)); */
    while (block_size < actual_size && level < LEVELS) {
        block_size <<= 1;
        level++;
    }
    return level;
}
size_t level_to_size(int level) {
    return MIN_BLOCK_SIZE << level;
}
header_t* split_block(header_t* block, int from_level, int to_level);
/**
 * Splits a larger block to get a block of the target level.
 *
 * @param target_level  The desired level (e.g., level 2 for 128B).
 * @return              Pointer to a free block, or NULL if no memory left.
 */
header_t* split_larger_block(int target_level) {
    // Search from target_level + 1 up to MAX_LEVELS
    for (int current_level = target_level + 1; current_level < MAX_LEVELS; current_level++) {
        header_t* larger_block = tb_find_free_block(current_level);

        if (larger_block != NULL) {
            // Remove the larger block from its free list
            free_list_remove(larger_block);

            // Split into two smaller buddies
            int smaller_level = current_level - 1;
            size_t smaller_size = level_to_size(smaller_level);

            // Buddy 1: Reuse the original block (but halve its size)
            larger_block->s.size = smaller_size;

            // Buddy 2: Compute address of the buddy
            header_t* buddy = (header_t*)((char*)larger_block + smaller_size);
            buddy->s.size = smaller_size;
            buddy->s.is_free = 1;

            // Add Buddy 2 to the free list
            free_list_add(buddy);

            // If we've reached the target level, return Buddy 1
            if (smaller_level == target_level) {
                return larger_block;
            } else {
                // Otherwise, keep splitting recursively
                return split_larger_block(target_level);
            }
        }
    }
    return NULL;  // No larger blocks available
}
void free_list_remove(header_t* block) {
    int level = size_to_level(block->s.size);
    header_t** curr = &free_lists[level];  // Pointer to the head pointer

    // Traverse the list to find the block
    while (*curr != NULL) {
        if (*curr == block) {
            *curr = block->s.next_free;  // Unlink the block
            break;
        }
        curr = &(*curr)->s.next_free;    // Move to next node
    }
}
/**
 * Merges a block with its free buddy (if available).
 *
 * @param block  The block to merge (must be free).
 * @return       The merged block (or original if no merge occurred).
 */
header_t* merge_buddies(header_t* block) {
    if (block == NULL || !block->s.is_free) {
        return block;  // Not free or invalid → no merge
    }

    int level = size_to_level(block->s.size);
    header_t* buddy = get_buddy(block, level);

    // Check if buddy exists, is free, and has the same size
    if (buddy != NULL && buddy->s.is_free &&
        buddy->s.size == block->s.size) {

        // Remove both blocks from their free list
        free_list_remove(block);
        free_list_remove(buddy);

        // Determine which buddy is the left (lower address) block
        header_t* left_block = (block < buddy) ? block : buddy;
        header_t* right_block = (block < buddy) ? buddy : block;

        // Merge into a larger block (double the size)
        left_block->s.size *= 2;
        left_block->s.is_free = 1;

        // Add the merged block to the higher-level free list
        free_list_add(left_block);

        // Recursively try merging the new larger block
        return merge_buddies(left_block);
    }

    return block;  // No merge possible
}
void calculate_total_size(size_t size) {
    return HEADER_SIZE + size;
};
void tb_free(void* ptr) {
    if (ptr == NULL) return;

    header_t* block = (header_t*)ptr - 1;  // Get header from payload
    block->s.is_free = 1;                  // Mark as free

    // Attempt to merge with buddies
    header_t* merged_block = merge_buddies(block);

    // Add the (possibly merged) block to the free list
    free_list_add(merged_block);
}
void tb_free(void* ptr) {
    /* Get the block's level using level = size_to_level(block->size) */
    /*Attempt to merge (coalesce) the block with its buddy (if the buddy is free).*/
    /* Insert the resulting (possibly merged) block into the correct free list. */


}
void* tb_request_memory(size_t size) {}
void* tb_calloc(size_t size);
void* tb_realloc(size_t size);
#endif // ALLOCATOR_H_
