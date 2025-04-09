#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

pthread_mutex_t lock;

// Low-priority thread function
void* low_priority_task(void* arg) {
    printf("Low-priority task: Locking resource...\n");
    pthread_mutex_lock(&lock);
    sleep(3);  // Simulate a long-running task
    printf("Low-priority task: Unlocking resource...\n");
    pthread_mutex_unlock(&lock);
    return NULL;
}

// High-priority thread function
void* high_priority_task(void* arg) {
    sleep(1);  // Ensure the low-priority task locks first
    printf("High-priority task: Waiting for resource...\n");
    pthread_mutex_lock(&lock);
    printf("High-priority task: Acquired resource!\n");
    pthread_mutex_unlock(&lock);
    return NULL;
}

// Medium-priority thread function
void* medium_priority_task(void* arg) {
    sleep(2);  // Interrupt before low-priority releases the lock
    printf("Medium-priority task: Running...\n");
    sleep(2);  // Simulate medium-priority execution delaying high-priority
    printf("Medium-priority task: Done.\n");
    return NULL;
}

void set_thread_priority(pthread_t thread, int priority) {
    struct sched_param param;
    param.sched_priority = priority;
    pthread_setschedparam(thread, SCHED_FIFO, &param);
}

int main() {
    pthread_t low, high, medium;
    pthread_mutex_init(&lock, NULL);

    // Create threads
    pthread_create(&low, NULL, low_priority_task, NULL);
    pthread_create(&high, NULL, high_priority_task, NULL);
    pthread_create(&medium, NULL, medium_priority_task, NULL);

    // Set thread priorities (higher number = higher priority)
    set_thread_priority(low, 10);
    set_thread_priority(high, 30);
    set_thread_priority(medium, 20);

    // Wait for threads to complete
    pthread_join(low, NULL);
    pthread_join(high, NULL);
    pthread_join(medium, NULL);

    pthread_mutex_destroy(&lock);
    return 0;
}
