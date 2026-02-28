#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMBER_OF_THREADS 8
pthread_barrier_t barrier_1;
pthread_barrier_t barrier_2;
pthread_barrier_t barrier_3;
void *three_stage_thread(void *arg) 
{
    int thread_id = *(int *)arg;
    printf("Thread %d is in stage 1\n", thread_id);
    sleep(1); // Simulate work in stage 1
    pthread_barrier_wait(&barrier_1);
    printf("Thread %d is in stage 2\n", thread_id);
    sleep(1); // Simulate work in stage 2
    pthread_barrier_wait(&barrier_2);
    printf("Thread %d is in stage 3\n", thread_id);
    sleep(1); // Simulate work in stage 3
    pthread_barrier_wait(&barrier_3);
    return NULL;
}

int main() 
{
    pthread_t threads[NUMBER_OF_THREADS];
    int thread_ids[NUMBER_OF_THREADS];
    pthread_barrier_init(&barrier_1, NULL, NUMBER_OF_THREADS);
    pthread_barrier_init(&barrier_2, NULL, NUMBER_OF_THREADS);
    pthread_barrier_init(&barrier_3, NULL, NUMBER_OF_THREADS);

    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, three_stage_thread, &thread_ids[i]);
    }

    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier_1);
    pthread_barrier_destroy(&barrier_2);
    pthread_barrier_destroy(&barrier_3);
    return 0;
}