/**
 * Assignment 1: Log Entry Processing
 * Processes 20000 log entries in parallel using OpenMP
 * 
 * Each log entry contains:
 * - request_id
 * - user_id
 * - response_time_ms
 * 
 * Classification:
 * - FAST: < 100 ms
 * - MEDIUM: 100-300 ms
 * - SLOW: > 300 ms
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

#define NUM_LOGS 20000
#define NUM_THREADS 4

// Log classification categories
typedef enum {
    FAST,    // < 100 ms
    MEDIUM,  // 100-300 ms
    SLOW     // > 300 ms
} LogCategory;

// Log entry structure
typedef struct {
    int request_id;
    int user_id;
    int response_time_ms;
    LogCategory category;
} LogEntry;

int main() {
    LogEntry *logs = (LogEntry *)malloc(NUM_LOGS * sizeof(LogEntry));
    if (logs == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    // Counters for each category
    int fast_count = 0;
    int medium_count = 0;
    int slow_count = 0;

    printf("=== Assignment 1: Log Entry Processing ===\n");
    printf("Processing %d log entries with %d threads\n\n", NUM_LOGS, NUM_THREADS);

    double start_time = omp_get_wtime();

    #pragma omp parallel num_threads(NUM_THREADS)
    {
        int thread_id = omp_get_thread_num();

        // Use single to initialize all logs (only one thread does this)
        #pragma omp single
        {
            printf("[Thread %d] Initializing %d log entries...\n", thread_id, NUM_LOGS);
            
            // Seed random number generator
            srand((unsigned int)time(NULL));
            
            for (int i = 0; i < NUM_LOGS; i++) {
                logs[i].request_id = i + 1;
                logs[i].user_id = (rand() % 1000) + 1;  // Random user_id between 1-1000
                logs[i].response_time_ms = rand() % 500; // Random response time 0-499 ms
                logs[i].category = FAST; // Default, will be classified later
            }
            
            printf("[Thread %d] Log initialization complete.\n\n", thread_id);
        }

        // Barrier to ensure all threads wait until logs are fully initialized
        #pragma omp barrier

        // Use for to process logs in parallel - classify each log
        #pragma omp for
        for (int i = 0; i < NUM_LOGS; i++) {
            if (logs[i].response_time_ms < 100) {
                logs[i].category = FAST;
            } else if (logs[i].response_time_ms <= 300) {
                logs[i].category = MEDIUM;
            } else {
                logs[i].category = SLOW;
            }
        }

        // Another barrier to ensure all threads finish processing before the summary
        #pragma omp barrier

        // Use single to compute and print summary (sequentially, no reduction)
        #pragma omp single
        {
            printf("[Thread %d] Computing summary (sequential count, no reduction)...\n", thread_id);
            
            // Count each category sequentially
            for (int i = 0; i < NUM_LOGS; i++) {
                switch (logs[i].category) {
                    case FAST:
                        fast_count++;
                        break;
                    case MEDIUM:
                        medium_count++;
                        break;
                    case SLOW:
                        slow_count++;
                        break;
                }
            }

            printf("\n=== Log Processing Summary ===\n");
            printf("Total logs processed: %d\n", NUM_LOGS);
            printf("FAST   (< 100 ms):    %d logs (%.2f%%)\n", 
                   fast_count, (float)fast_count / NUM_LOGS * 100);
            printf("MEDIUM (100-300 ms):  %d logs (%.2f%%)\n", 
                   medium_count, (float)medium_count / NUM_LOGS * 100);
            printf("SLOW   (> 300 ms):    %d logs (%.2f%%)\n", 
                   slow_count, (float)slow_count / NUM_LOGS * 100);
        }
    }

    double end_time = omp_get_wtime();
    
    printf("\n=== Performance ===\n");
    printf("Total execution time: %.4f seconds\n", end_time - start_time);

    // Print some sample logs for verification
    printf("\n=== Sample Log Entries ===\n");
    printf("%-12s %-10s %-15s %-10s\n", "Request ID", "User ID", "Response (ms)", "Category");
    printf("--------------------------------------------------\n");
    for (int i = 0; i < 10; i++) {
        const char *cat_str;
        switch (logs[i].category) {
            case FAST:   cat_str = "FAST";   break;
            case MEDIUM: cat_str = "MEDIUM"; break;
            case SLOW:   cat_str = "SLOW";   break;
            default:     cat_str = "UNKNOWN"; break;
        }
        printf("%-12d %-10d %-15d %-10s\n", 
               logs[i].request_id, logs[i].user_id, 
               logs[i].response_time_ms, cat_str);
    }
    printf("... (%d more entries)\n", NUM_LOGS - 10);

    free(logs);
    return 0;
}
