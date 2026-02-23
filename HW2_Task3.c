#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define NUMBER_OF_THREADS 10
#define ARRAY_SIZE 50000000

// Task 3: Parallel Maximum Search: Write a C program that finds the maximum value in an array 
// using multiple threads. Similar to the previous task, divide the array into equal parts and 
// assign each part to a thread. Each thread should find the maximum value in its assigned portion, 
// and the main thread should collect the results and determine the overall maximum value.

int array[ARRAY_SIZE];

typedef struct{
    int start_index;
    int end_index;
    int thread_id;
    int local_max;
} ThreadData;

// sequential solution for finding maximum value in an array
int findMax()
{
    int max = array[0];
    for (int i = 1; i < ARRAY_SIZE; i++)
    {
       if(array[i] > max)
       {
           max = array[i];
       }     
    }
    return max;
}

// Helper function to get elapsed time in seconds
double get_elapsed_time(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
}

// Threaded solution for finding maximum value in an array
void* threadedMaxSearch(void* arg)
{
    ThreadData *data = (ThreadData *)arg;
    int start_index = data->start_index;
    int end_index = data->end_index;
    int thread_id = data->thread_id;
    
    int local_max = array[start_index];
    for (int i = start_index + 1; i < end_index; i++) 
    {
        if (array[i] > local_max) 
        {
            local_max = array[i];
        }
    }
    
    data->local_max = local_max;
    return NULL;
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
    
    // Sequential maximum search timed using wall clock
    struct timeval start_seq, end_seq;
    gettimeofday(&start_seq, NULL);
    int max_value = findMax(); // sequential solution
    gettimeofday(&end_seq, NULL);
    double seq_time = get_elapsed_time(start_seq, end_seq);
    printf("Maximum value (sequential): %d\n", max_value);
    printf("Sequential time: %f seconds\n", seq_time);
    
    // Parallel maximum search timed using wall clock
    struct timeval start_par, end_par;
    gettimeofday(&start_par, NULL);
    // creating threads for maximum search
    for (int i = 0; i < NUMBER_OF_THREADS; i++) 
    {
        thread_data[i].start_index = i * (ARRAY_SIZE / NUMBER_OF_THREADS);
        thread_data[i].end_index = (i + 1) * (ARRAY_SIZE / NUMBER_OF_THREADS);
        thread_data[i].thread_id = i;
        pthread_create(&threads[i], NULL, threadedMaxSearch, &thread_data[i]);
    }
    for(int i = 0; i < NUMBER_OF_THREADS; i++) 
    {
        pthread_join(threads[i], NULL);
    }
    gettimeofday(&end_par, NULL);
    double par_time = get_elapsed_time(start_par, end_par);

    // compute overall maximum from thread local maxima
    int max_parallel = thread_data[0].local_max;
    for(int i = 1; i < NUMBER_OF_THREADS; i++) 
    {
        if (thread_data[i].local_max > max_parallel) 
        {
            max_parallel = thread_data[i].local_max;
        }
    }

    printf("Maximum value (parallel): %d\n", max_parallel);
    printf("Parallel time: %f seconds\n", par_time);
    
    return 0;
}