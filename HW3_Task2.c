#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
pthread_barrier_t barrier;
#define NUMBER_OF_THREADS 5

void * lobby_thread(void * arg)
{
    int thread_id = *(int *)arg;
    usleep((rand() % 5000) * 1000);  // Sleep for 0-5000 milliseconds
    printf("Thread %d is waiting at the barrier\n", thread_id);
    pthread_barrier_wait(&barrier);
    printf("Thread %d Game Started\n", thread_id);
    return NULL;
}

int main()
{
    pthread_t threads[NUMBER_OF_THREADS];
    int thread_ids[NUMBER_OF_THREADS];
    pthread_barrier_init(&barrier, NULL, NUMBER_OF_THREADS);

    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, lobby_thread, &thread_ids[i]);
    }

    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);
    return 0;
}
