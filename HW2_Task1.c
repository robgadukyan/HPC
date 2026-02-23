#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUMBER_OF_THREADS 3

// Task 1: Create a simple C program that creates multiple threads using the pthread library. 
// Each thread should print its own thread ID and then exit.

void* thread_function(void* arg) 
{
    // printing thread ID
    pthread_t thread_id = pthread_self();
    printf("Thread %lu is running\n", (unsigned long)thread_id);
    return NULL;
}

int main() 
{
    pthread_t threads[NUMBER_OF_THREADS];
    
    // creating threads
    for (int i = 0; i < NUMBER_OF_THREADS; i++) 
    {
        if (pthread_create(&threads[i], NULL, thread_function, NULL) != 0)
        {
            perror("Failed to create thread");
            return 1;
        }
    }
    
    // waiting for the threads to finish
    for (int i = 0; i < NUMBER_OF_THREADS; i++) 
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}