#include <stdio.h>
#include <pthread.h>
#include <unistd.h> // For sleep function

#define NUM_THREADS 3

pthread_barrier_t barrier;

void* thread_func(void* id) {
    int tid = *(int*)id;
    printf("thread %d doing work before barrier\n", tid);
    sleep(1); //simulate work

    // Wait on the barrier 
    printf("thread %d is in barrier \n", tid);
    int res = pthread_barrier_wait(&barrier);
	
    //threads should not reach this point until all threads in barrier
    if (res == PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("thread %d is serial thread + passed barrier\n", tid);
    } else {
        printf("thread %d passed barrier\n", tid);
    }

    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    //barrier init
    if (pthread_barrier_init(&barrier, NULL, NUM_THREADS) != 0) {
        printf("could not create barrier\n");
        return -1;
    }

    //create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1; 		//thread ids from 1 to NUM_THREADS
        if (pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]) != 0) {
            printf("could not create thread %d\n", i);
            return -1;
        }
    }

    //join threads
    for (int i = 0; i < NUM_THREADS; i++)  pthread_join(threads[i], NULL);

    //reuse same barrier
    printf("\nREUSE BARRIER:\n"); 
    //create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1 + NUM_THREADS; 		//thread ids from NUM_THREADS+1 to NUMTHREADS*2
        if (pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]) != 0) {
            printf("could not create thread %d\n", i);
            return -1;
        }
    }

    //join threads
    for (int i = 0; i < NUM_THREADS; i++)  pthread_join(threads[i], NULL);

    //destroy barrier
    if(pthread_barrier_destroy(&barrier)!=0) {
	    printf("could not delete barrier\n");
	    return -1;
    }

    //reinitialize barrier
    printf("\nREINITIALIZING DESTROYED BARRIER:\n"); 
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    //create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1 + NUM_THREADS*2; 		//thread ids from NUM_THREADS*2+1 to NUMTHREADS*3
        if (pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]) != 0) {
            printf("could not create thread %d\n", i);
            return -1;
        }
    }

    //join threads
    for(int i = 0; i < NUM_THREADS; i++) pthread_join(threads[i], NULL);


    return 0;
}
