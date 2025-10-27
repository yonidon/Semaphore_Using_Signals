// Created by Johnatan Dvoishes, Student ID 207375940
// Operating systems course Winter 2026
// Idea and design was conceived by me. LLM was used to help a little with the thread and signal code.


// ========= SIGNAL BASED IMPLEMENTATION OF SEMAPHORES =========
// A special thread sends SIGUSR1 signal to the process
// The worker thread that catches it checks if the semaphore is up
// If it's up, the worker sets it down and enters critical region
// Then it sets the sempahore back up and allows other threads to set it

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define THREAD_COUNT 20
#define DOWN 0
#define UP 1

int semaphore = UP;                    // Binary semaphore
pthread_t threads[THREAD_COUNT];       // worker threads that receive the signal
pthread_t sender_thread;               // signal-sending thread

// === Semaphore operations ===
void sem_init(int status)  { semaphore = status; }
void sem_down(void)        { semaphore = DOWN;   }
void sem_up(void)          { semaphore = UP;     }

// === Function for thread that sends the signals ===
void *signal_sending_thread(void *arg) {
    (void)arg;

    // Ignore SIGUSR1 in this thread
    struct sigaction sa_ignore;
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;
    sigaction(SIGUSR1, &sa_ignore, NULL);

    printf("[Sender] Sending SIGUSR1 to all threads...\n\n");
    sleep(1);

    //Signal self. The thread  who catches the signal wins.
    for (int i = 0; i < THREAD_COUNT; i++) {
        kill(getpid(), SIGUSR1);
        sleep(1);
    }

}

//=== Function for threads that receive the signals ===
void *signal_receiving_thread(void *arg) {
    int thread_num = *(int *)arg;
    free(arg);

    // Start synchronous wait for SIGUSR1
    sigset_t waitset;
    sigemptyset(&waitset);
    sigaddset(&waitset, SIGUSR1);
    int sig;
    int result = sigwait(&waitset, &sig);

    // If the thread catches the signal it enters the critical region
    if (result == 0 && sig == SIGUSR1 && semaphore == UP) {
        sem_down(); // Lock the Semaphore
        printf("[Thread %2d] Entering critical region...\n", thread_num);
        sleep(2);   // Do some work...
        printf("[Thread %2d] Leaving critical region...\n\n", thread_num);
        sem_up();   // Release Semaphore
    }

}

int main(void) {

    // Initialize semaphore
    sem_init(UP);

    // Block the signal so the threads could catch it synchronously
    sigset_t blockset;
    sigemptyset(&blockset);
    sigaddset(&blockset, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &blockset, NULL);

    // The main thread ignores the SIGUSR1 signal
    struct sigaction sa_ignore;
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;
    sigaction(SIGUSR1, &sa_ignore, NULL);

    // Create worker threads that receive the signal
    for (int i = 0; i < THREAD_COUNT; i++) {
        int *thread_num = malloc(sizeof(int));
        *thread_num = i + 1;
        pthread_create(&threads[i], NULL, signal_receiving_thread, thread_num);
    }

    // Create sender thread
    pthread_create(&sender_thread, NULL, signal_sending_thread, NULL);

    // Wait for all threads to finish
    pthread_join(sender_thread, NULL);
    for (int i = 0; i < THREAD_COUNT; i++)
        pthread_join(threads[i], NULL);

    printf("[Main] All threads finished.\n");
    return EXIT_SUCCESS;
}
