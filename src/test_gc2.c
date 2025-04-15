#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>
#include <stdint.h>
#include <time.h>
#include <sys/resource.h>
#include <stdint.h>
#include "tb_gc.h"

#define MAX_ITERATIONS 100000
#define CHILDREN_PER_NODE 10
#define CLEAR_THRESHOLD 10000
#define TASK_DELAY_MICROS 1000

typedef struct object {
    size_t size;
    uint8_t marked;
    struct object **children;
    size_t child_count;
    struct object *next_object;
} object_t;

/* extern object_t *all_objects; */

object_t *extract_object(void *user_ptr, size_t child_count) {
    return (object_t *)((uint8_t *)user_ptr - sizeof(object_t) - sizeof(object_t *) * child_count);
}

long get_memory_usage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;  // Return in KB
}

// Function to simulate a periodic task with sleep
void simulate_task() {
    usleep(TASK_DELAY_MICROS);
}

// Function to output header to CSV file
void output_csv_header(FILE *csv_file) {
    fprintf(csv_file, "Iteration,Task Delay (us),GC Runs,Memory Usage (KB)\n");
}

// Function to output results to CSV file
void output_csv(FILE *csv_file, int iteration, long task_delay, int gc_runs, long memory_usage) {
    fprintf(csv_file, "%d,%ld,%d,%ld\n", iteration, task_delay, gc_runs, memory_usage);
}
int main() {
    printf("Starting GC benchmark...\n");

    // Open the CSV file for writing
    FILE *csv_file = fopen("gc_benchmark.csv", "w");
    if (!csv_file) {
        perror("Error opening CSV file");
        return 1;
    }

    output_csv_header(csv_file);  // Write CSV header

    gc_init();

    // Track GC statistics
    int gc_runs = 0;
    struct timespec start_time, end_time;

    // Root set (simulated)
    object_t *roots[MAX_ITERATIONS];
    int root_count = 0;

    // Main benchmark loop
    for (int i = 0; i < MAX_ITERATIONS; i++) {
        // Step 1: Allocate a node with children
        void *user_data = gc_alloc(32, CHILDREN_PER_NODE);  // Allocate object
        object_t *obj = extract_object(user_data, CHILDREN_PER_NODE);  // Get object header

        // Step 2: Add to root set
        gc_add_root(obj);
        roots[root_count++] = obj;

        // Step 3: Simulate task and measure task delay
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        simulate_task();
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        long task_delay_us = (end_time.tv_nsec - start_time.tv_nsec) / 1000;  // Convert to microseconds

        // Step 4: Periodically clear roots to trigger GC
        if (root_count > CLEAR_THRESHOLD) {
            // Clear roots to simulate the end of a task and make objects unreachable
            gc_remove_root(roots[0]);  // Remove the first root object
            for (int j = 1; j < root_count; j++) {
                gc_remove_root(roots[j]);
            }
            root_count = 0;
            gc_collect_full();
            gc_runs++;  // Count a GC run
        }

        // Step 5: Optionally print memory usage
        if (i % 10000 == 0) {
            long memory_usage = get_memory_usage();  // Get memory usage
            output_csv(csv_file, i, task_delay_us, gc_runs, memory_usage);  // Write results to CSV
        }

        // Step 6: Periodically run GC
        if (i % 5000 == 0) {
            gc_collect_full();  // Run a full GC cycle
            gc_runs++;  // Count the GC run
        }
    }

    // Final output to CSV
    long memory_usage = get_memory_usage();
    output_csv(csv_file, MAX_ITERATIONS, 0, gc_runs, memory_usage);  // Write final results to CSV

    // Close the CSV file
    fclose(csv_file);

    printf("GC benchmark complete! Results written to gc_benchmark.csv\n");
    return 0;
}
