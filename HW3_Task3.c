#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUMBER_OF_SENSORS 5

pthread_barrier_t barrier;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

float sensor_data[NUMBER_OF_SENSORS] = {0.0}; // Array to store temperature data from each sensor

void* sensor_thread(void* arg) 
{
    int sensor_id = *(int*)arg; // Get the sensor ID
    
    // Simulate collecting temperature data (random value between 15-35 Celsius)
    float temperature = 15.0 + (rand() % 210) / 10.0; // Random temp between 15-36°C
    
    // Lock mutex to safely store the temperature
    pthread_mutex_lock(&mutex);
    sensor_data[sensor_id] = temperature;
    printf("Sensor %d collected temperature: %.1f°C\n", sensor_id, temperature);
    pthread_mutex_unlock(&mutex);
    
    // Wait for all sensors to finish collecting data
    printf("Sensor %d waiting at barrier...\n", sensor_id);
    pthread_barrier_wait(&barrier);
    printf("Sensor %d passed the barrier, proceeding to processing\n", sensor_id);
    
    return NULL;
}

int main() 
{
    pthread_t threads[NUMBER_OF_SENSORS];
    int sensor_ids[NUMBER_OF_SENSORS];
    
    srand(time(NULL)); // Seed random number generator
    
    // Initialize barrier with NUMBER_OF_SENSORS + 1 (sensors + main thread)
    pthread_barrier_init(&barrier, NULL, NUMBER_OF_SENSORS);
    pthread_mutex_init(&mutex, NULL);
    
    printf("=== Distributed Weather Station ===\n");
    printf("Creating %d sensor threads...\n\n", NUMBER_OF_SENSORS);
    
    // Create sensor threads
    for (int i = 0; i < NUMBER_OF_SENSORS; i++) 
    {
        sensor_ids[i] = i;
        if (pthread_create(&threads[i], NULL, sensor_thread, (void*)&sensor_ids[i]) != 0)
        {
            perror("Failed to create thread");
            return 1;
        }
    }
    
    // Wait for all threads to finish
    for (int i = 0; i < NUMBER_OF_SENSORS; i++) 
    {
        pthread_join(threads[i], NULL);
    }
    
    // Calculate and display average temperature
    float sum = 0.0;
    printf("\n=== Data Processing ===\n");
    printf("All sensors have synchronized. Temperature readings:\n");
    for (int i = 0; i < NUMBER_OF_SENSORS; i++) 
    {
        printf("Sensor %d: %.1f°C\n", i, sensor_data[i]);
        sum += sensor_data[i];
    }
    
    float average = sum / NUMBER_OF_SENSORS;
    printf("\nAverage temperature: %.2f°C\n", average);
    
    // Cleanup
    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);
    
    return 0;
}