#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_barrier_t barrier;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define NUMBER_OF_PLAYERS 5
int array[NUMBER_OF_PLAYERS] = {0}; // Array to store dice values for each player
// Task 1:  multi-threaded dice game
void throw_dice(void * arg) 
{
    int player_id = *(int*)arg; // Get the player ID from the argument
    int dice_value = rand() % 6 + 1; // Simulate throwing a dice (1-6)
    pthread_mutex_lock(&mutex); // Lock the mutex to safely update the shared array
    array[player_id] += dice_value; // Store the dice value for the player
    pthread_mutex_unlock(&mutex); // Unlock the mutex after updating the array
    pthread_barrier_wait(&barrier); // Wait for all players to throw their dice

    printf("Thread %d threw a dice and got: %d\n", player_id, dice_value);
}

int determine_winner() 
{
    int max_value = 0;
    int winner_thread_id = 0;
    for (int i = 0; i < NUMBER_OF_PLAYERS; i++) 
    {
        if (array[i] > max_value) 
        {
            max_value = array[i];
            winner_thread_id = i;
        }
    }
    return winner_thread_id;
}

int main() 
{
    pthread_t threads[NUMBER_OF_PLAYERS];
    int player_ids[NUMBER_OF_PLAYERS]; // Array to store player IDs
    pthread_barrier_init(&barrier, NULL, NUMBER_OF_PLAYERS);
    pthread_mutex_init(&mutex, NULL);
    int number_of_rounds = 3; // Number of rounds to play
    for(int i = 0; i < number_of_rounds; i++) 
    {
        printf("Round %d:\n", i + 1);
        // Create threads for each player
        for (int j = 0; j < NUMBER_OF_PLAYERS; j++) 
        {
            player_ids[j] = j; // Store player ID in array
            if (pthread_create(&threads[j], NULL, (void*)throw_dice, (void*)&player_ids[j]) != 0) 
            {
                perror("Failed to create thread");
                return 1;
            }
        }

        // Wait for all threads to finish
        for (int j = 0; j < NUMBER_OF_PLAYERS; j++) 
        {
            pthread_join(threads[j], NULL);
        }
    }
    
    int winner = determine_winner();
    printf("The winner is thread %d with a dice value of %d\n", winner, array[winner]);
    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);
    return 0;
}