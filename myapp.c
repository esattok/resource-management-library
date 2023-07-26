#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include "rm.h"

#define NUMR 6     // number of resource types
#define NUMP 4     // number of threads

// Global Variables
int AVOID = 1;
int exist[NUMR] =  {8, 6, 7, 5, 9, 4};  // resources existing in the system

// Function Signatures
void* threadfunc0(void*);
void* threadfunc1(void*);
void* threadfunc2(void*);
void* threadfunc3(void*);

// Main Function
int main(int argc, char **argv) {
    int tids[NUMP];
    char tempStr[100];
    pthread_t threadArray[NUMP];
    int ret = 0;

    if (argc != 2) {
        printf ("usage: ./myapp avoidflag\n");
        exit (1);
    }

    AVOID = atoi(argv[1]);
    rm_init(NUMP, NUMR, exist, AVOID); // Initialize the library

    for (int i = 0; i < NUMP; i++) {
        tids[i] = i;
    }

    rm_print_state("Initial State");

    // Create threads
    pthread_create(&(threadArray[0]), NULL, (void*) threadfunc0, (void*) &(tids[0]));
    pthread_create(&(threadArray[1]), NULL, (void*) threadfunc1, (void*) &(tids[1]));
    pthread_create(&(threadArray[2]), NULL, (void*) threadfunc2, (void*) &(tids[2]));
    pthread_create(&(threadArray[3]), NULL, (void*) threadfunc3, (void*) &(tids[3]));

    if (AVOID == 0) {
        while (1) {
            ret = rm_detection();
            if (ret > 0) {
                sprintf(tempStr, "Deadlock Happened (%d Processes are Deadlocked). Terminating...", ret);
                rm_print_state(tempStr);
                break;
            }
            sleep(1);
        }
    }

    // If no deadlocked happened then wait for join otherwise terminate
    if (ret == 0) {
        for (int i = 0; i < NUMP; i++) {
            pthread_join (threadArray[i], NULL);
        }
    }
    
    return 0;
}

// Thread Functions
void* threadfunc0(void* a) {
    if (AVOID == 0) {
        int tid;
        int request1[MAXR] = {2, 1, 5, 3, 4, 2};
        int request2[MAXR] = {0, 0, 3, 0, 0, 0};
        tid = *((int*)a);

        rm_thread_started(tid); // Let the library know that thread is started
        sleep(1);

        rm_request(request1); // Make the first request
        rm_print_state("After First Request of Thread 0");
        sleep(5);

        rm_request(request2);
        rm_print_state("After Second Request of Thread 0");

        rm_release(request1);
        rm_release(request2);
        rm_print_state("After First and Second Release of Thread 0");

        rm_thread_ended(); // Let the library know that thread is ended
        pthread_exit(NULL);
    }

    else {
        int tid;
        int request1[MAXR] = {2, 1, 3, 2, 3, 2};
        int request2[MAXR] = {0, 1, 0, 0, 0, 0};
        int claim[MAXR] = {3, 2, 6, 4, 5, 3};
        tid = *((int*)a);

        rm_thread_started(tid); // Let the library know that thread is started
        rm_claim (claim);
        sleep(1);

        rm_request(request1); // Make the first request
        rm_print_state("After First Request of Thread 0");
        sleep(6);

        rm_request(request2); // Make the second request
        rm_print_state("After Second Request of Thread 0 (Waited by Avoidance)");

        rm_release(request1);
        rm_release(request2);
        rm_print_state("After First and Second Release of Thread 0");

        rm_thread_ended(); // Let the library know that thread is ended
        pthread_exit(NULL);
    }
    
}

void* threadfunc1(void* a) {
    if (AVOID == 0) {
        int tid;
        int request1[MAXR] = {3, 2, 0, 1, 2, 0};
        tid = *((int*)a);
        
        rm_thread_started(tid); // Let the library know that thread is started
        sleep(2);

        rm_request(request1); // Make the first request
        rm_print_state("After First Request of Thread 1");
        sleep(5);

        rm_release(request1);
        rm_print_state("After First Release of Thread 1");

        rm_thread_ended(); // Let the library know that thread is ended
        pthread_exit(NULL);
    }
    else {
        int tid;
        int request1[MAXR] = {2, 2, 1, 1, 2, 1};
        int claim[MAXR] = {4, 3, 3, 2, 2, 1};
        tid = *((int*)a);

        rm_thread_started(tid); // Let the library know that thread is started
        rm_claim(claim);
        sleep(2);

        rm_request(request1); // Make the first request
        rm_print_state("After First Request of Thread 1");
        sleep(7);

        rm_release(request1);        
        rm_print_state("After First Release of Thread 1");

        rm_thread_ended(); // Let the library know that thread is ended
        pthread_exit(NULL);
    }
    
}

void* threadfunc2(void* a) {
    if (AVOID == 0) {
        int tid;
        int request1[MAXR] = {2, 2, 1, 0, 1, 1};
        tid = *((int*)a);

        rm_thread_started(tid); // Let the library know that thread is started
        sleep(3);

        rm_request(request1); // Make the first request
        rm_print_state("After First Request of Thread 2");
        sleep(5);

        rm_release(request1);
        rm_print_state("After First Release of Thread 2");

        rm_thread_ended(); // Let the library know that thread is ended
        pthread_exit(NULL);
    }
    else {
        int tid;
        int request1[MAXR] = {2, 2, 1, 0, 1, 1};
        int claim[MAXR] = {2, 3, 1, 1, 2, 3};
        tid = *((int*)a);

        
        rm_thread_started(tid); // Let the library know that thread is started
        rm_claim (claim);
        sleep(3);

        rm_request (request1); // Make the first request
        rm_print_state("After First Request of Thread 2");
        sleep(7);

        rm_release (request1);
        rm_print_state("After First Release of Thread 2");

        rm_thread_ended(); // Let the library know that thread is ended
        pthread_exit(NULL);
    }
}

void* threadfunc3(void* a) {
    if (AVOID == 0) {
        int tid;
        int request1[MAXR] = {1, 1, 1, 1, 2, 1};
        tid = *((int*)a);

        rm_thread_started(tid); // Let the library know that thread is started
        sleep(4);

        rm_request (request1); // Make the first request
        rm_print_state("After First Request of Thread 3");
        sleep(5);

        rm_release (request1);
        rm_print_state("After First Release of Thread 3");

        rm_thread_ended(); // Let the library know that thread is ended
        pthread_exit(NULL);
    }
    else {
        int tid;
        int request1[MAXR] = {0, 0, 0, 1, 0, 0};
        int claim[MAXR] = {1, 1, 1, 1, 1, 1};
        tid = *((int*)a);
        
        rm_thread_started(tid); // Let the library know that thread is started
        rm_claim(claim);
        sleep(4);

        rm_request(request1); // Make the first request
        rm_print_state("After First Request of Thread 3");
        sleep(7);

        rm_release(request1);
        rm_print_state("After First Release of Thread 3");

        rm_thread_ended(); // Let the library know that thread is ended
        pthread_exit(NULL);
    }
}
