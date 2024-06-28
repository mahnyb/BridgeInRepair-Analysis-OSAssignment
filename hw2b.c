// Mahny Barazandehtar - 20210702004 - CSE 331 - Assignment 2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h> // For gettimeofday
#include <sys/wait.h> // For wait
#include <sys/ipc.h> // For IPC
#include <sys/shm.h> // For shared memory
#include <semaphore.h> // For POSIX semaphores

#define MAX_CARS 300000

// Define a structure to hold shared variables
struct SharedState {
    sem_t mutex;
    sem_t bridge;
    int bridgeCapacity;
    int carsOnBridge;
    int carDirections[2]; // Tracks the number of cars going in each direction
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <bridgeCapacity> <totalCars>\n", argv[0]);
        return 1;
    }

    int bridgeCapacity = atoi(argv[1]);
    int totalCars = atoi(argv[2]);

    // Allocate shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(struct SharedState), IPC_CREAT | 0666);
    struct SharedState* shared = (struct SharedState*) shmat(shmid, NULL, 0);

    // Initialize semaphores
    sem_init(&shared->mutex, 1, 1);
    sem_init(&shared->bridge, 1, bridgeCapacity);
    shared->bridgeCapacity = bridgeCapacity;
    shared->carsOnBridge = 0;
    shared->carDirections[0] = 0;
    shared->carDirections[1] = 0;

    struct timeval startTime, endTime;
    gettimeofday(&startTime, NULL);

    // Fork child processes
    for (int i = 0; i < totalCars; i++) {
        pid_t pid = fork();
        if (pid == 0) { 
            srand(getpid()); // Ensure different seed for each process
            int direction = rand() % 2; // 0 for GO_LEFT, 1 for GO_RIGHT
            Arrive(i + 1, direction, shared);
            usleep(1000); // Simulate crossing time
            Depart(i + 1, direction, shared);
            exit(0);
        }
    }

    // Wait for all children to dieeee muahahaha :D
    while (wait(NULL) > 0);

    // Cleanup semaphores
    sem_destroy(&shared->mutex);
    sem_destroy(&shared->bridge);

    // Detach and remove shared memory
    shmdt(shared);
    shmctl(shmid, IPC_RMID, NULL);

    gettimeofday(&endTime, NULL);
    long seconds = (endTime.tv_sec - startTime.tv_sec);
    long microseconds = ((seconds * 1000000) + endTime.tv_usec) - (startTime.tv_usec);
    double elapsed = microseconds / 1000000.0;

    printf("\nExecution time: %.6f seconds\n", elapsed); 

    return 0;
}

void Arrive(int carID, int direction, struct SharedState *shared) {
    sem_wait(&shared->mutex);
    while (shared->carsOnBridge == shared->bridgeCapacity || (shared->carsOnBridge > 0 && shared->carDirections[1-direction] > 0)) {
        sem_post(&shared->mutex); 
        usleep(1000); // Sleep briefly to prevent busy waiting
        sem_wait(&shared->mutex); 
    }
    shared->carDirections[direction]++;
    shared->carsOnBridge++;
    printf("Car %d arrived on the bridge going %s. Current cars: %d\n", carID, direction == 0 ? "left" : "right", shared->carsOnBridge);
    sem_post(&shared->mutex); // Unlock after updating conditions
}

void Depart(int carID, int direction, struct SharedState *shared) {
    sem_wait(&shared->mutex);
    shared->carsOnBridge--;
    shared->carDirections[direction]--;
    printf("Car %d departed from the bridge going %s. Remaining cars: %d\n", carID, direction == 0 ? "left" : "right", shared->carsOnBridge);
    sem_post(&shared->mutex); // Unlock after updating conditions
    sem_post(&shared->bridge); // Signal that the bridge has capacity
}
