// Author    : Johnatan Dvoishes 
// Student ID: 207375940 
// Operating systems course Winter 2026

// ========= SIGNAL BASED IMPLEMENTATION OF SEMAPHORES =========
// A special thread sends SIGUSR1 signal to the process
// The worker thread that catches it checks if the semaphore is up
// If it's up, the worker sets it down and enters critical region
// Then it sets the sempahore back up and allows other threads to set it

#define _POSIX_C_SOURCE 200809L // Use posix threads
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
void *signal_sending_thread() {

    // Ignore SIGUSR1 in this thread, since it's only used to send the signals
    struct sigaction sa_ignore;
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;
    sigaction(SIGUSR1, &sa_ignore, NULL);

    printf("[Sender] Sending SIGUSR1 to all threads...\n\n");
    sleep(1);

    //Signal everyone in the process. The thread who catches the signal wins.
    while(1) {
        kill(getpid(), SIGUSR1);
        sleep(1);
    }

}

//=== Function for threads that receive the signals ===
void *signal_receiving_thread(void *arg) {

    // Get thread number
    int thread_num = *(int *)arg;

    // Start synchronous wait for SIGUSR1
    sigset_t waitset;
    sigemptyset(&waitset);
    sigaddset(&waitset, SIGUSR1);
    int signal_id;
    
    // Loop until able to enter critical region
    while(1){
      // If the thread catches the signal and semaphore is free, it enters the critical region
      int result = sigwait(&waitset, &signal_id);
      if (result == 0 && signal_id == SIGUSR1 && semaphore == UP) {
          sem_down(); // Lock the Semaphore
          printf("[Thread %2d] Entering critical region...\n", thread_num);
          sleep(1);   // Do some work...
          printf("[Thread %2d] Leaving critical region...\n\n", thread_num);
          sem_up();   // Release Semaphore
          break;
      }
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

    // The main thread ignores the SIGUSR1 signal since it's not a worker thread
    struct sigaction sa_ignore;
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;
    sigaction(SIGUSR1, &sa_ignore, NULL);

    // Create worker threads that receive the signal
    for (int i = 1; i <= THREAD_COUNT; i++) {
        int *thread_num = malloc(sizeof(int));// Malloc to prevent race condition where two threads get same thread number 
	*thread_num = i;
        pthread_create(&threads[i], NULL, signal_receiving_thread, thread_num);
    }

    // Create sender thread that will send SIGUSR1 to the process
    pthread_create(&sender_thread, NULL, signal_sending_thread, NULL);

    // Wait for all threads to finish
    for (int i = 0; i < THREAD_COUNT; i++)
        pthread_join(threads[i], NULL);
        
    // Kill the signal sending thread
    sleep(1);
    pthread_cancel(sender_thread);
    pthread_join(sender_thread, NULL);

    printf("[Main] All threads finished.\n");
    return EXIT_SUCCESS;
}
