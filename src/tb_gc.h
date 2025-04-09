#ifndef GC_H_
#define GC_H_

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


void gc_init();

void *gc_malloc(size_t size);

void gc_collect();

void gc_shutdown();

#endif // GC_H_
