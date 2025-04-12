#ifndef TB_ALLOCATOR_H
#define TB_ALLOCATOR_H

#include <stddef.h>
#include <pthread.h>
#include <sys/mman.h>

/* Configuration constants */
#define ALIGNMENT 16
#define HEAP_SIZE (1 << 20)  // 1MB
#define MIN_BLOCK_SIZE (1 << 5)  // 32 bytes
#define MAX_BLOCK_SIZE (1 << 20) // 1MB

/* Public interface */
void tb_initialize_allocator(void);
void* tb_malloc(size_t size);
void tb_free(void* ptr);
void tb_cleanup_allocator(void);

#endif // TB_ALLOCATOR_H
