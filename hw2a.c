// Mahny Barazandehtar - 20210702004 - CSE 331 - Assignment 2

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h> // Include for gettimeofday

#define MAX_CARS 300000

// Semaphores and shared variables
sem_t bridge;
sem_t mutex; 
int bridgeCapacity;
int carsOnBridge = 0;
int carDirections[2] = {0, 0}; // Tracks the number of cars going in each direction

void *car(void *arg);
void Arrive(int carID, int direction);
void Depart(int carID, int direction);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <bridgeCapacity> <totalCars>\n", argv[0]);
        return 1;
    }

    bridgeCapacity = atoi(argv[1]);
    int totalCars = atoi(argv[2]);
    pthread_t cars[MAX_CARS];

    struct timeval startTime, endTime; // Variables to hold start and end times

    // Start timing
    gettimeofday(&startTime, NULL);

    // Initialize semaphores
    sem_init(&bridge, 0, bridgeCapacity);
    sem_init(&mutex, 0, 1); // Initialize mutex for critical section

    // Create car threads
    for (int i = 0; i < totalCars; i++) {
        int *info = malloc(2 * sizeof(int));
        info[0] = i + 1; // Car ID
        info[1] = rand() % 2; // Direction: 0 for GO_LEFT, 1 for GO_RIGHT
        pthread_create(&cars[i], NULL, car, info);
    }

    // Wait for all car threads to finish
    for (int i = 0; i < totalCars; i++) {
        pthread_join(cars[i], NULL);
    }

    // Cleanup semaphores
    sem_destroy(&bridge);
    sem_destroy(&mutex);

    // End timing
    gettimeofday(&endTime, NULL);

    // Calculate execution time
    long seconds = (endTime.tv_sec - startTime.tv_sec);
    long microseconds = ((seconds * 1000000) + endTime.tv_usec) - (startTime.tv_usec);
    double elapsed = microseconds / 1000000.0;

    printf("\nExecution time: %.6f seconds\n", elapsed);

    return 0;
}

void *car(void *arg) {
    int carID = ((int*)arg)[0];
    int direction = ((int*)arg)[1];
    free(arg);

    Arrive(carID, direction);
    usleep(1000); // Simulate crossing time
    Depart(carID, direction);

    return NULL;
}

void Arrive(int carID, int direction) {
    sem_wait(&mutex); // Lock to protect shared resource access
    
    // Wait until the bridge is available for this direction
    while (carsOnBridge == bridgeCapacity || (carsOnBridge > 0 && carDirections[1-direction] > 0)) {
        sem_post(&mutex); 
        usleep(1000); // Sleep briefly to prevent busy waiting
        sem_wait(&mutex); 
    }
    carDirections[direction]++;
    carsOnBridge++;
    printf("Car %d arrived on the bridge going %s. Current cars: %d\n", carID, direction == 0 ? "left" : "right", carsOnBridge);
    sem_post(&mutex); // Unlock after updating conditions
}

void Depart(int carID, int direction) {
    sem_wait(&mutex); // Lock to protect shared resource access
    carsOnBridge--;
    carDirections[direction]--;
    printf("Car %d departed from the bridge going %s. Remaining cars: %d\n", carID, direction == 0 ? "left" : "right", carsOnBridge);
    sem_post(&mutex); // Unlock after updating conditions
    sem_post(&bridge); // Signal that the bridge has capacity
}
