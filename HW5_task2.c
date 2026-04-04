/**
 * Assignment 2: Delivery Priority Update
 * Processes 10000 delivery orders in parallel using OpenMP
 * 
 * Each order has:
 * - order_id
 * - distance_km
 * - priority
 * 
 * Priority Rules:
 * - HIGH: distance_km < threshold
 * - NORMAL: distance_km >= threshold
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

#define NUM_ORDERS 10000
#define NUM_THREADS 4

// Priority levels
typedef enum {
    UNASSIGNED,
    NORMAL,
    HIGH
} Priority;

// Delivery order structure
typedef struct {
    int order_id;
    float distance_km;
    Priority priority;
} DeliveryOrder;

// Shared threshold variable
float distance_threshold;

// Array to store per-thread HIGH priority counts
int thread_high_count[NUM_THREADS];

int main() {
    DeliveryOrder *orders = (DeliveryOrder *)malloc(NUM_ORDERS * sizeof(DeliveryOrder));
    if (orders == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    // Initialize thread_high_count array
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_high_count[i] = 0;
    }

    printf("=== Assignment 2: Delivery Priority Update ===\n");
    printf("Processing %d delivery orders with %d threads\n\n", NUM_ORDERS, NUM_THREADS);

    // Initialize orders with random data before parallel region
    srand((unsigned int)time(NULL));
    for (int i = 0; i < NUM_ORDERS; i++) {
        orders[i].order_id = i + 1;
        orders[i].distance_km = (float)(rand() % 500) / 10.0f;  // 0.0 to 49.9 km
        orders[i].priority = UNASSIGNED;
    }

    double start_time = omp_get_wtime();

    #pragma omp parallel num_threads(NUM_THREADS)
    {
        int thread_id = omp_get_thread_num();

        // Use single to set one common rule for all orders (distance threshold)
        #pragma omp single
        {
            distance_threshold = 20.0f;  // 20 km threshold
            printf("[Thread %d] Setting common distance threshold: %.1f km\n", 
                   thread_id, distance_threshold);
            printf("[Thread %d] Rule: distance < %.1f km -> HIGH priority\n", 
                   thread_id, distance_threshold);
            printf("[Thread %d]       distance >= %.1f km -> NORMAL priority\n\n", 
                   thread_id, distance_threshold);
        }
        // Implicit barrier after single

        // Use for to process orders in parallel - assign priorities
        #pragma omp for
        for (int i = 0; i < NUM_ORDERS; i++) {
            if (orders[i].distance_km < distance_threshold) {
                orders[i].priority = HIGH;
            } else {
                orders[i].priority = NORMAL;
            }
        }

        // Barrier to ensure all priorities are assigned
        #pragma omp barrier

        // Use single to print a message that priority assignment is finished
        #pragma omp single
        {
            printf("[Thread %d] Priority assignment complete for all %d orders.\n\n", 
                   thread_id, NUM_ORDERS);
        }
        // Implicit barrier after single

        // Use another for to count HIGH priority orders per thread
        #pragma omp for
        for (int i = 0; i < NUM_ORDERS; i++) {
            if (orders[i].priority == HIGH) {
                thread_high_count[thread_id]++;
            }
        }

        // Another barrier ensures all per-thread counts are ready
        #pragma omp barrier

        // Use single to print the values from thread_high_count[] and total
        #pragma omp single
        {
            printf("[Thread %d] Printing per-thread HIGH priority counts:\n", thread_id);
            printf("\n=== Per-Thread HIGH Priority Counts ===\n");
            
            int total_high = 0;
            for (int t = 0; t < NUM_THREADS; t++) {
                printf("Thread %d: %d HIGH priority orders\n", t, thread_high_count[t]);
                total_high += thread_high_count[t];
            }
            
            printf("\n=== Summary ===\n");
            printf("Total HIGH priority orders: %d\n", total_high);
            printf("Total NORMAL priority orders: %d\n", NUM_ORDERS - total_high);
            printf("HIGH priority percentage: %.2f%%\n", (float)total_high / NUM_ORDERS * 100);
        }
    }

    double end_time = omp_get_wtime();

    printf("\n=== Performance ===\n");
    printf("Total execution time: %.4f seconds\n", end_time - start_time);

    // Print some sample orders for verification
    printf("\n=== Sample Delivery Orders ===\n");
    printf("%-10s %-15s %-10s\n", "Order ID", "Distance (km)", "Priority");
    printf("--------------------------------------\n");
    for (int i = 0; i < 15; i++) {
        const char *priority_str;
        switch (orders[i].priority) {
            case HIGH:       priority_str = "HIGH";       break;
            case NORMAL:     priority_str = "NORMAL";     break;
            case UNASSIGNED: priority_str = "UNASSIGNED"; break;
            default:         priority_str = "UNKNOWN";    break;
        }
        printf("%-10d %-15.1f %-10s\n", 
               orders[i].order_id, orders[i].distance_km, priority_str);
    }
    printf("... (%d more orders)\n", NUM_ORDERS - 15);

    free(orders);
    return 0;
}
