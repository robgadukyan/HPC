#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define NUMBER_OF_THREADS 20
#define ARRAY_SIZE 50000000

// Task 2: Parallel Array Summation: Write a C program that divides an array into equal parts 
// and assigns each part to a separate thread for summation. Each thread should compute the sum 
// of its assigned portion of the array, and the main thread should collect the results and 
// compute the total sum.

int array[ARRAY_SIZE];

typedef struct {
    int start_index;
    int end_index;
    int thread_id;
    long long partial_sum;
} ThreadData;

void *computesum(void *arg) 
{
    ThreadData *data = (ThreadData *)arg;
    int start_index = data->start_index;
    int end_index = data->end_index;
    
    long long partial_sum = 0;
    for (int i = start_index; i < end_index; i++) 
    {
        partial_sum += array[i];
    }
    
    data->partial_sum = partial_sum;
    return NULL;
}

// Sequential sum function
long long sequential_sum() {
    long long sum = 0;
    for (int i = 0; i < ARRAY_SIZE; i++) {
        sum += array[i];
    }
    return sum;
}

// Helper function to get elapsed time in seconds
double get_elapsed_time(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
}

int main()
{
    pthread_t threads[NUMBER_OF_THREADS];
    ThreadData thread_data[NUMBER_OF_THREADS];
    
    // initializing the array with random values
    for (int i = 0; i < ARRAY_SIZE; i++) 
    {
        array[i] = rand() % 1024200; // random values between 0 and 1024199
    }
    
    // Sequential version with timing
    struct timeval start_seq, end_seq;
    gettimeofday(&start_seq, NULL);
    long long seq_sum = sequential_sum();
    gettimeofday(&end_seq, NULL);
    double seq_time = get_elapsed_time(start_seq, end_seq);
    
    printf("Sequential sum: %lld\n", seq_sum);
    printf("Sequential time: %f seconds\n", seq_time);
    
    // Parallel version with timing
    struct timeval start_par, end_par;
    gettimeofday(&start_par, NULL);
    long long total_sum = 0;

    // creating threads for summation
    for (int i = 0; i < NUMBER_OF_THREADS; i++)
    {
        thread_data[i].start_index = i * (ARRAY_SIZE / NUMBER_OF_THREADS);
        thread_data[i].end_index = (i + 1) * (ARRAY_SIZE / NUMBER_OF_THREADS);
        thread_data[i].thread_id = i;
        
        // creating a thread for each portion of the array
        pthread_create(&threads[i], NULL, computesum, &thread_data[i]);
    }

    // waiting for the threads to finish
    for (int i = 0; i < NUMBER_OF_THREADS; i++) 
    {
        pthread_join(threads[i], NULL);
    }

    // calculating the total sum
    for (int i = 0; i < NUMBER_OF_THREADS; i++) 
    {
        total_sum += thread_data[i].partial_sum;
    }
    
    gettimeofday(&end_par, NULL);
    double par_time = get_elapsed_time(start_par, end_par);

    printf("Parallel sum: %lld\n", total_sum);
    printf("Parallel time: %f seconds\n", par_time);
    
    return 0;
}