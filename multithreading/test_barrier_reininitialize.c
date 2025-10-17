#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 20

pthread_barrier_t barrier;

void *thread_func(void *arg)
{
    int id = *(int *)arg;
    int result;

    // First barrier
    result = pthread_barrier_wait(&barrier);
    if (result == PTHREAD_BARRIER_SERIAL_THREAD)
    {
        printf("Serial thread %d passed the first barrier.\n", id);
    }
    else
    {
        printf("Thread %d passed the first barrier.\n", id);
    }


    // Wait for all threads to sync after the first use
    pthread_barrier_wait(&barrier);

    // Only the last thread performs the reinitialization
    if (id == NUM_THREADS - 1)
    {
        pthread_barrier_destroy(&barrier);
        pthread_barrier_init(&barrier, NULL, (NUM_THREADS - 1));
        printf("Barrier reinitialized by thread %d.\n", id);
    }

    // Reinitialized barrier
    pthread_barrier_wait(&barrier);

    // Second use of the reinitialized barrier
    result = pthread_barrier_wait(&barrier);
    if (result == PTHREAD_BARRIER_SERIAL_THREAD)
    {
        printf("Serial thread %d passed the reinitialized barrier.\n", id);
    }
    else
    {
        printf("Thread %d passed the reinitialized barrier.\n", id);
    }

    return NULL;
}

int main()
{
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];
    int i;

    // Initialize the barrier
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    // Create threads
    for (i = 0; i < NUM_THREADS; i++)
    {
        ids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &ids[i]);
    }

    // Join threads
    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);

    return 0;
}
