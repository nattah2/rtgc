#include "allocator.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

typedef char ALIGN[16];

union alignas(16) header {
    struct {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    ALIGN stub;
}; // Ensures 16-byte alignment (C11/C++11);
typedef union header header_t;

#define HEADER_SIZE sizeof(header_t)

extern header_t* head = NULL, *tail = NULL;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

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


void *tb_malloc(size_t size) {
    // Sanity check - make sure we get something usable
    if (!size) return NULL;
    // Find the smallest power of 2 size that can hold both a header and
    size = (size + HEADER_SIZE + 15) & ~15;
    // Find a free block (sequentially allocated?)
    pthread_mutex_lock(&lock);
    header_t* block = tb_find_free_block(size);
    // If we find one then mark it as unfree?
    if (block) {
        block->s.is_free = 0;
        pthread_mutex_unlock(&lock);
        return (void*)(block + 1); // Return memory after the header.
    }
    // If we get here, then we didn't find a free space.
    block = tb_request_memory(size);

    // I don't know what this does?
    if (block == (void*) -1) {
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    // If no free block is found, request more memory.
    return;
    // If requesting more memory doesn't work, then return null;
    return NULL;
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
