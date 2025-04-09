#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

pthread_mutex_t lock;
void* allocated_objects[10];  // Simulating object allocations
int allocation_index = 0;

// Simple mock function to simulate memory allocation
void* gc_malloc(size_t size) {
    if (allocation_index < 10) {
        allocated_objects[allocation_index] = malloc(size);
        return allocated_objects[allocation_index++];
    }
    return NULL; // Simulating out-of-memory scenario
}

// Simple mock function to simulate garbage collection
void gc_run() {
    printf("[GC] Running garbage collection...\n");
    for (int i = 0; i < allocation_index; i++) {
        if (allocated_objects[i]) {
            free(allocated_objects[i]);
            allocated_objects[i] = NULL;
        }
    }
    allocation_index = 0;
    printf("[GC] Garbage collection complete.\n");
}

// Background GC thread
void* gc_thread(void* arg) {
    while (1) {
        sleep(4);  // Run GC periodically
        gc_run();
    }
    return NULL;
}

// Low-priority task (allocates memory and holds lock)
void* low_priority_task(void* arg) {
    printf("Low-priority task: Locking resource...\n");
    pthread_mutex_lock(&lock);
    void* obj = gc_malloc(128);
    printf("Low-priority task: Allocated memory at %p\n", obj);
    sleep(3);  // Simulate long execution
    printf("Low-priority task: Unlocking resource...\n");
    pthread_mutex_unlock(&lock);
    return NULL;
}

// High-priority task (blocked if priority inversion occurs)
void* high_priority_task(void* arg) {
    sleep(1);
    printf("High-priority task: Waiting for resource...\n");
    pthread_mutex_lock(&lock);
    printf("High-priority task: Acquired resource!\n");
    pthread_mutex_unlock(&lock);
    return NULL;
}

// Medium-priority task (causes priority inversion)
void* medium_priority_task(void* arg) {
    sleep(2);
    printf("Medium-priority task: Running...\n");
    sleep(2);
    printf("Medium-priority task: Done.\n");
    return NULL;
}

void set_thread_priority(pthread_t thread, int priority) {
    struct sched_param param;
    param.sched_priority = priority;
    pthread_setschedparam(thread, SCHED_FIFO, &param);
}

int main() {
    pthread_t low, high, medium, gc;
    pthread_mutex_init(&lock, NULL);

    // Start GC in the background
    pthread_create(&gc, NULL, gc_thread, NULL);

    // Create worker threads
    pthread_create(&low, NULL, low_priority_task, NULL);
    pthread_create(&high, NULL, high_priority_task, NULL);
    pthread_create(&medium, NULL, medium_priority_task, NULL);

    // Set thread priorities
    set_thread_priority(low, 10);
    set_thread_priority(high, 30);
    set_thread_priority(medium, 20);

    // Wait for threads to complete
    pthread_join(low, NULL);
    pthread_join(high, NULL);
    pthread_join(medium, NULL);

    // Cleanup
    pthread_cancel(gc);
    pthread_mutex_destroy(&lock);
    return 0;
}
