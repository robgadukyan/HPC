#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

#define NUMBER_OF_THREADS 15

// Task 5: CPU Core Exploration: Explore which CPU cores threads are assigned to and 
// demonstrate thread execution across different cores.

void* heavyLoop(void* arg) 
{
    long long sum = 0;
    printf("Thread %lu is running a heavy loop.\n", (unsigned long)pthread_self());
    printf("This thread is running on CPU core: %d\n", sched_getcpu());
    for (long long i = 0; i < 1000000000; i++) 
    {
        sum += i;
    }

    return NULL;
}

int main()
{
    pthread_t threads[NUMBER_OF_THREADS];
    
    // Create threads for heavy loop execution
    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_create(&threads[i], NULL, heavyLoop, NULL);
    }

    // Wait for threads to finish
    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}