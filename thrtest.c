#include <stdio.h>
#include <pthread.h>

#define THREAD_COUNT 3

// The function executed by each thread
void *thread_function(void *arg)
{
    int *array = (int *)arg; // Cast argument to int pointer
    int thread_num = array[0];
    int thread_arg = array[1];
    printf("Thread %d received argument: %d\n", thread_num, thread_arg);
    pthread_exit(NULL);
}

int main()
{
    pthread_t threads[THREAD_COUNT];
    int thread_args[THREAD_COUNT][2] = {{1, 100}, {2, 200}, {3, 300}}; // Array of arguments to pass to threads

    // Create multiple threads
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_create(&threads[i], NULL, thread_function, thread_args[i]);
    }

    // Wait for the threads to finish
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        printf("%d\n", pthread_tryjoin_np(threads[i], NULL));
    }

    printf("All threads have finished\n");

    return 0;
}
