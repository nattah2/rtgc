#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#define ALIGNMENT 16
#define HEADER_SIZE sizeof(header_t)

#define HEAP_SIZE (1 << 20)  // 1MB heap
#define MIN_BLOCK_SIZE (1 << 5)  // 32 bytes
#define MAX_BLOCK_SIZE (1 << 20) // 1MB
#define LEVELS (__builtin_ctz(MAX_BLOCK_SIZE) - __builtin_ctz(MIN_BLOCK_SIZE) + 1)

// Special header for oversized allocations
typedef struct oversized_header {
    size_t size;
    unsigned is_free;
    void* address;  // Original mmap address
    struct oversized_header* next;
} oversized_header_t;

typedef union header {
    struct {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    uint8_t padding[ALIGNMENT];
} header_t;

static header_t* free_lists[LEVELS] = { NULL };
static oversized_header_t* oversized_blocks = NULL;
static pthread_mutex_t allocator_lock = PTHREAD_MUTEX_INITIALIZER;
static void* base_address = NULL;
static int allocator_initialized = 0;

static inline size_t align_up(size_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

static int size_to_level(size_t size) {
    size_t actual_size = align_up(size + HEADER_SIZE);
    int level = 0;
    size_t block_size = MIN_BLOCK_SIZE;
    while (block_size < actual_size && level < LEVELS - 1) {
        block_size <<= 1;
        level++;
    }
    return level;
}

static size_t level_to_size(int level) {
    return MIN_BLOCK_SIZE << level;
}

void* tb_request_memory(size_t size) {
    void *block = mmap(NULL, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return block == MAP_FAILED ? NULL : block;
}

void tb_initialize_allocator() {
    if (allocator_initialized) return;

    void *heap = tb_request_memory(HEAP_SIZE);
    if (!heap) {
        perror("Failed to initialize memory allocator");
        return;
    }
    base_address = heap;

    header_t *block = (header_t*)heap;
    block->s.size = HEAP_SIZE - HEADER_SIZE;
    block->s.is_free = 1;
    block->s.next = NULL;

    free_lists[LEVELS - 1] = block;
    allocator_initialized = 1;
}

// Handle large allocations that exceed MAX_BLOCK_SIZE
void* tb_malloc_large(size_t size) {
    // Allocate memory for both the header and the requested size
    size_t total_size = size + sizeof(oversized_header_t);
    void* mem = tb_request_memory(total_size);
    if (!mem) return NULL;

    // Setup the oversized header
    oversized_header_t* header = (oversized_header_t*)mem;
    header->size = size;
    header->is_free = 0;
    header->address = mem;  // Store the original pointer for freeing later

    // Add to the oversized blocks list
    pthread_mutex_lock(&allocator_lock);
    header->next = oversized_blocks;
    oversized_blocks = header;
    pthread_mutex_unlock(&allocator_lock);

    // Return the usable memory area
    return (void*)(header + 1);
}

void* tb_malloc(size_t size) {
    if (!size) return NULL;

    // Initialize allocator if not already done
    if (!allocator_initialized) {
        tb_initialize_allocator();
        if (!allocator_initialized) return NULL;
    }

    // Check if allocation is too large for the buddy system
    if (size > MAX_BLOCK_SIZE - HEADER_SIZE) {
        return tb_malloc_large(size);
    }

    pthread_mutex_lock(&allocator_lock);

    int level = size_to_level(size);
    int i = level;
    while (i < LEVELS && free_lists[i] == NULL) i++;

    if (i == LEVELS) {
        pthread_mutex_unlock(&allocator_lock);
        return NULL;
    }

    header_t *block = free_lists[i];
    free_lists[i] = block->s.next;
    size_t block_size = level_to_size(i);

    while (i > level) {
        i--;
        block_size >>= 1;
        header_t *buddy = (header_t*)((char*)block + block_size);
        buddy->s.size = block_size - HEADER_SIZE;
        buddy->s.is_free = 1;
        buddy->s.next = free_lists[i];
        free_lists[i] = buddy;
    }

    block->s.size = level_to_size(level) - HEADER_SIZE;
    block->s.is_free = 0;
    pthread_mutex_unlock(&allocator_lock);
    return (void*)(block + 1);
}

// Free a large allocation
void tb_free_large(oversized_header_t* header) {
    pthread_mutex_lock(&allocator_lock);

    // Remove from the oversized blocks list
    oversized_header_t** pp = &oversized_blocks;
    while (*pp && *pp != header) {
        pp = &(*pp)->next;
    }

    if (*pp) {
        *pp = header->next;
    }

    pthread_mutex_unlock(&allocator_lock);

    // Return the memory to the OS
    munmap(header->address, header->size + sizeof(oversized_header_t));
}

void tb_free(void *ptr) {
    if (!ptr) return;

    // Check if this might be a large allocation
    uintptr_t ptr_addr = (uintptr_t)ptr;
    if (ptr_addr < (uintptr_t)base_address ||
        ptr_addr >= (uintptr_t)base_address + HEAP_SIZE) {

        // This is likely a large allocation
        oversized_header_t* header = (oversized_header_t*)ptr - 1;

        // Validate that this is indeed one of our oversized blocks
        pthread_mutex_lock(&allocator_lock);
        oversized_header_t* curr = oversized_blocks;
        int is_valid = 0;

        while (curr) {
            if (curr == header) {
                is_valid = 1;
                break;
            }
            curr = curr->next;
        }
        pthread_mutex_unlock(&allocator_lock);

        if (is_valid) {
            tb_free_large(header);
            return;
        }
        // If not valid, fall through to regular free
    }

    header_t *block = (header_t*)ptr - 1;
    size_t block_size = block->s.size + HEADER_SIZE;
    int level = size_to_level(block->s.size);

    pthread_mutex_lock(&allocator_lock);
    block->s.is_free = 1;

    while (level < LEVELS - 1) {
        uintptr_t offset = (uintptr_t)block - (uintptr_t)base_address;
        uintptr_t buddy_offset = offset ^ block_size;
        header_t *buddy = (header_t*)((uintptr_t)base_address + buddy_offset);

        if ((uintptr_t) buddy < (uintptr_t) base_address ||
            (uintptr_t) buddy >= (uintptr_t)base_address + HEAP_SIZE ||
            !buddy->s.is_free || buddy->s.size != block->s.size) {
            break;
        }

        header_t **prev = &free_lists[level];
        while (*prev && *prev != buddy) {
            prev = &(*prev)->s.next;
        }
        if (*prev == buddy) {
            *prev = buddy->s.next;
        } else {
            break;
        }

        if ((uintptr_t)block > (uintptr_t)buddy) {
            block = buddy;
        }
        block_size <<= 1;
        block->s.size = block_size - HEADER_SIZE;
        level++;
    }

    block->s.next = free_lists[level];
    free_lists[level] = block;
    pthread_mutex_unlock(&allocator_lock);
}

// Function to clean up the allocator (useful for preventing memory leaks)
void tb_cleanup_allocator() {
    pthread_mutex_lock(&allocator_lock);

    // Free all oversized blocks
    oversized_header_t* curr = oversized_blocks;
    while (curr) {
        oversized_header_t* next = curr->next;
        munmap(curr->address, curr->size + sizeof(oversized_header_t));
        curr = next;
    }
    oversized_blocks = NULL;

    // Free the main heap
    if (base_address) {
        munmap(base_address, HEAP_SIZE);
        base_address = NULL;
    }

    // Reset free lists
    for (int i = 0; i < LEVELS; i++) {
        free_lists[i] = NULL;
    }

    allocator_initialized = 0;
    pthread_mutex_unlock(&allocator_lock);
}
