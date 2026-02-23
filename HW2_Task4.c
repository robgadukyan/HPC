#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define NUMBER_OF_THREADS 8
#define MAX_NUMBER 20000000

// Task 4: Prime number check: Write a C program that checks how many prime numbers are there 
// in a given range (1 to 20 million) using multiple threads. Each thread should check a portion of the range 
// and count the number of prime numbers in that portion. The main thread should collect the 
// results and compute the total count of prime numbers in the range.

typedef struct{
    int start_index;
    int end_index;
    int thread_id;
    int local_max; // reusing this field to store prime count
} ThreadData;

int isPrime(int num){
    if(num <= 1) return 0; // 0 and 1 are not prime numbers
    long long limit = (long long)sqrt((long double)num); // checking up to the square root of the number
    for(int i = 2; i <= limit; i++){
        if(num % i == 0) return 0; // number is divisible by another number, hence not prime
    }
    return 1; // number is prime
}

int count_primes_in_range(int start, int end) 
{
    int prime_count = 0;
    for (int i = start; i < end; i++) 
    {
        if (isPrime(i)) 
        {
            prime_count++;
        }
    }
    return prime_count;
}

// Sequential prime counting
int sequential_prime_count() {
    int count = 0;
    for (int i = 1; i <= MAX_NUMBER; i++) {
        if (isPrime(i)) {
            count++;
        }
    }
    return count;
}

// Helper function to get elapsed time in seconds
double get_elapsed_time(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
}

// parallel solution for counting prime numbers in a given range
void* primeCheck(void *arg){
    ThreadData* data = (ThreadData *)arg;
    int start_index = data->start_index;
    int end_index = data->end_index;
    int thread_id = data->thread_id;
    int prime_count = 0;

    for(int i = start_index; i <= end_index; i++){
        if(isPrime(i)){
            prime_count++;
        }
    }

    // store the count in the ThreadData structure and return NULL
    data->local_max = prime_count;
    return NULL;
}

int main()
{
    pthread_t threads[NUMBER_OF_THREADS];
    ThreadData thread_data[NUMBER_OF_THREADS];
    
    // Sequential version with timing
    struct timeval start_seq, end_seq;
    gettimeofday(&start_seq, NULL);
    int seq_count = sequential_prime_count();
    gettimeofday(&end_seq, NULL);
    double seq_time = get_elapsed_time(start_seq, end_seq);
    
    printf("Sequential prime count: %d\n", seq_count);
    printf("Sequential time: %f seconds\n", seq_time);
    
    // Parallel version with timing
    struct timeval start_par, end_par;
    gettimeofday(&start_par, NULL);
    
    // Create threads for prime checking
    for(int i = 0 ; i< NUMBER_OF_THREADS; i++){
        thread_data[i].start_index = i * (MAX_NUMBER / NUMBER_OF_THREADS) + 1;
        thread_data[i].end_index = (i + 1) * (MAX_NUMBER / NUMBER_OF_THREADS);
        thread_data[i].thread_id = i;
        pthread_create(&threads[i], NULL, primeCheck, &thread_data[i]);
    }

    // Wait for threads to finish
    for(int i = 0; i < NUMBER_OF_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }
    
    // Collect results
    int total_prime_count = 0;
    for(int i = 0; i < NUMBER_OF_THREADS; i++)
    {
        total_prime_count += thread_data[i].local_max;
    }
    
    gettimeofday(&end_par, NULL);
    double par_time = get_elapsed_time(start_par, end_par);
    
    printf("Parallel prime count: %d\n", total_prime_count);
    printf("Parallel time: %f seconds\n", par_time);
    
    return 0;
}