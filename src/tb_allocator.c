#include "tb_allocator.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <glibc.h>

typedef char ALIGN[16];

#define HEADER_SIZE sizeof(header_t)

#define MIN_BLOCK_SIZE 32
#define MIN_BLOCK_SIZE 1 << 20
#define MAX_LEVELs 7

extern header_t* head = NULL, *tail = NULL;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
/* struct Block { */
/*     struct Block* next; // Pointer to next free block */
/* }; */
header_t* free_lists[MAX_LEVELS];

header_t *tb_find_free_block(size_t size) {
   header_t* curr = head;
   while (curr) {
       if (curr->s.is_free && curr->s.size >= size) {
           return curr;
       }
       curr = curr->s.next;
   }
   return NULL;
}

static header_t* tb_request_memory(size_t size) {
    void* block = srbk(0); // Get current heap break
    if (sbrk(size) == (void*)-1)
        return null;
    header_t* hdr = (header_t*) block;
    hdr->s.size = size;
    hdr->s.is_free = 0;
    hdr->s.next = NULL;

    // thread unsafe??
    if (!head)
        head = hdr;
    if (tail)
        tail->s.next = hdr;
    tail = hdr;
    return hdr;
}

size_t size_to_level(size):
    order = ceil(log2(total_size));  // Round up to next power of 2
    if order < MIN_ORDER{
        order = MIN_ORDER
    }
    return order
}

void *tb_malloc(size_t size) {
    // Sanity check - make sure we get something usable
    if (!size) return NULL;
    // Find the smallest power of 2 size that can hold both a header and
    total_size = (size + HEADER_SIZE + 15) & ~15;
    // Find a free block (sequentially allocated?)
    pthread_mutex_lock(&lock);
    header_t* block = tb_find_free_block(total_size);
    // If we find one then mark it as unfree?
    if (block) {
        block->s.is_free = 0;
        pthread_mutex_unlock(&lock);
        return (void*)(block + 1); // Return memory after the header.
    }
    // If we get here, then we didn't find a free space.
    block = tb_request_memory(total_size);
    if (!block) {
    return null
    }
    else {
        block->s.is_free = 0;
    }
    // If no free block is found, request more memory.
    // If requesting more memory doesn't work, then return null;
    return (void*)(block + 1); // Return memory after the header.
}

void tb_free(void* ptr) {
    if (!ptr) return;
    pthread_mutex_lock(&lock);
    header_t* hdr = (header_t*)ptr - 1;
    hdr->s.is_free = 1;
    // Coalesce with next block if free
    if (hdr->s.next && hdr->s.next->s.is_free) {
        hdr->s.size += HEADER_SIZE + hdr->s.next->s.size;
        hdr->s.next = hdr->s.next->s.next;
    }
    pthread_mutex_unlock(&lock);
}

void *tb_calloc(size_t size) {
    return;
}

void *tb_realloc(size_t size) {
    return;
}
