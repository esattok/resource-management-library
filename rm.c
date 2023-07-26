#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "rm.h"


// global variables

int DA;  // indicates if deadlocks will be avoided or not
int N;   // number of processes (threads)
int M;   // number of resource types
int ThreadFinish[MAXP]; // Indicates if a thread is finished or not (1 = Finished, 0 = Not Finished)
int ExistingRes[MAXR]; // Existing resources vector
int AvailableRes[MAXR]; // Available resources vector
int AllocationMat[MAXP][MAXR]; // Num of resources of each type allocated to each thread 
int RequestMat[MAXP][MAXR]; // Num of resources that are requested by a thread 
int MaxDemandMat[MAXP][MAXR]; // Max demand for each resource type for each thread 
int NeedMat[MAXP][MAXR]; // Need for each resource type for each thread 
pthread_t threadList[MAXP]; // Each index represents the user defined thread id and the value represents the real id

pthread_mutex_t mutex; // single mutex lock
pthread_cond_t cond; // condition variable for each thread

// end of global variables

// Extra function signatures
int safety_check();

// Functions

int rm_thread_started(int tid)
{
    /* Critical section starts here */
    pthread_mutex_lock(&mutex);

    if ((tid < 0) || (tid >= N)) {
        /* critical section end */
	    pthread_mutex_unlock(&mutex);

        return -1;
    }

    threadList[tid] = pthread_self(); // assign the real thread_id
    ThreadFinish[tid] = 0; // Thread is started fo mark it as not finished

    /* critical section end */
	pthread_mutex_unlock(&mutex);
    
    return 0;
}

int rm_thread_ended()
{
    /* Critical section starts here */
    pthread_mutex_lock(&mutex);

    // find the user defined thread_id
    int user_defined_id = -1;
    for (int i = 0; i < N; i++) {
        if (threadList[i] == pthread_self()) {
            user_defined_id = i;
        }
    }

    // Conditions that the function has an error
    if (user_defined_id == -1) {
        /* critical section end */
	    pthread_mutex_unlock(&mutex);

        return -1;
    }

    ThreadFinish[user_defined_id] = 1; // Thread is ended so mark it as finished

    /* critical section end */
	pthread_mutex_unlock(&mutex);

    return 0;
}

int rm_claim (int claim[])
{
    /* Critical section starts here */
    pthread_mutex_lock(&mutex);

    // If deadlock avoidance will not be used (the detection will be used) then this function is not applicable
    if (DA == 0) {
        /* critical section end */
	    pthread_mutex_unlock(&mutex);

        return -1;
    }

    // Find the user defined thread_id
    int user_defined_id = -1;
    for (int i = 0; i < N; i++) {
        if (threadList[i] == pthread_self()) {
            user_defined_id = i;
        }
    }

    // Conditions that the function has an error
    if (user_defined_id == -1) {
        /* critical section end */
	    pthread_mutex_unlock(&mutex);

        return -1;
    }

    // Succesfully populate the max demand info for the specified thread if the demand is not more than existing
    for (int i = 0; i < M; i++) {
        if (claim[i] > ExistingRes[i]) {
            /* critical section end */
	        pthread_mutex_unlock(&mutex);

            return -1;
        }
        MaxDemandMat[user_defined_id][i] = claim[i];
        NeedMat[user_defined_id][i] = MaxDemandMat[user_defined_id][i];
    }

    /* critical section end */
	pthread_mutex_unlock(&mutex);
    
    return 0;
}

// There is no synchronization needed in this function since only the main thread will call this function before any other thread is created
int rm_init(int p_count, int r_count, int r_exist[],  int avoid)
{
    DA = avoid;
    N = p_count;
    M = r_count;

    // Return -1 if invalid
    if (N > MAXP || N < 1 || M > MAXR || M < 1) {
        return -1;
    }

    // initialize Existing and Available vectors
    for (int i = 0; i < M; i++) {
        // Return -1 if invalid
        if (r_exist[i] < 0) {
            return -1;
        }

        ExistingRes[i] = r_exist[i];
        AvailableRes[i] = r_exist[i];
    }
    
    // Initialize allocation, max demand, and request matrices to 0
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            AllocationMat[i][j] = 0;
            MaxDemandMat[i][j] = 0;
            RequestMat[i][j] = 0;
            NeedMat[i][j] = 0;
        }

        ThreadFinish[i] = 1; // Initially there is no active thread so mark all as finished
    }

    // Initialize mutex and condition variables
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    
    return 0;
}


int rm_request (int request[])
{
    /* critical section start */
	pthread_mutex_lock(&mutex);

    // Find the user defined id of the calling thread
    int user_defined_id = -1;
    for (int i = 0; i < N; i++) {
        if (threadList[i] == pthread_self()) {
            user_defined_id = i;
        }
    }

    // Conditions that the function has an error
    if (user_defined_id == -1) {
        /* critical section end */
	    pthread_mutex_unlock(&mutex);

        return -1;
    }

    // Return error if the requested resources are more than the existing ones
    for (int i = 0; i < M; i++) {
        if (request[i] > ExistingRes[i]) {
            /* critical section end */
	        pthread_mutex_unlock(&mutex);

            return -1;
        }
    }

    // If there is no avoidance then just allocate the resources when available
    if (DA == 0) {
        for (int j = 0; j < M; j++) {
            RequestMat[user_defined_id][j] = request[j]; // Fill the request matrix
        }

        // Check if there is enough evailable resources
        for (int i = 0; i < M; i++) {
            if (RequestMat[user_defined_id][i] > AvailableRes[i]) {
                pthread_cond_wait(&cond, &mutex); // If there is an unavailable resource, wait
                i = -1; // After wake up from the wait, go back and check all resources again
            }
        }

        // If the thread is here, then we are sure there are available resources
        // Go to new state
        for (int i = 0; i < M; i++) {
            AvailableRes[i] = AvailableRes[i] - request[i];
            AllocationMat[user_defined_id][i] = AllocationMat[user_defined_id][i] + request[i];
            RequestMat[user_defined_id][i] = 0; // Request is completed
        }

        /* critical section end */
	    pthread_mutex_unlock(&mutex);

        return 0; // Return with success
    }

    // Populate the need vector for the current thread
    for (int i = 0; i < M; i++) {
        NeedMat[user_defined_id][i] = MaxDemandMat[user_defined_id][i] - AllocationMat[user_defined_id][i];
        RequestMat[user_defined_id][i] = request[i];

        // Check if the request is smaller than the need for the process
        if (RequestMat[user_defined_id][i] > NeedMat[user_defined_id][i]) {
            /* critical section end */
	        pthread_mutex_unlock(&mutex);

            return -1; // If the thread requests more resource than its max then there is an error 
        }
    } // Initialization and checks are done for deadlock avoidance

    // Check if there is enough available resources
    for (int i = 0; i < M; i++) {
        if (RequestMat[user_defined_id][i] > AvailableRes[i]) {
            pthread_cond_wait(&cond, &mutex); // If there is an unavailable resource, wait
            i = -1; // After wake up from the wait, go back and check all resources again
        }
    }
    // If the thread is here, then we are sure there are available resources

    // Pretend to go into the new state
    for (int i = 0; i < M; i++) {
        AvailableRes[i] = AvailableRes[i] - RequestMat[user_defined_id][i];
        AllocationMat[user_defined_id][i] = AllocationMat[user_defined_id][i] + RequestMat[user_defined_id][i];
        NeedMat[user_defined_id][i] = NeedMat[user_defined_id][i] - RequestMat[user_defined_id][i];
    }

    // Running the safety check algorithm on new state
    int isSafeState;
    isSafeState = safety_check();

    // If error occured in safety_check
    if (isSafeState == -1) {
        // Roll back to old state
        for (int i = 0; i < M; i++) {
            AvailableRes[i] = AvailableRes[i] + RequestMat[user_defined_id][i];
            AllocationMat[user_defined_id][i] = AllocationMat[user_defined_id][i] - RequestMat[user_defined_id][i];
            NeedMat[user_defined_id][i] = NeedMat[user_defined_id][i] + RequestMat[user_defined_id][i];
            RequestMat[user_defined_id][i] = 0;
        }

        /* critical section end */
	    pthread_mutex_unlock(&mutex);

        return -1;
    }
    

    while (safety_check() != 1) {
        // Roll back to old state
        for (int i = 0; i < M; i++) {
            AvailableRes[i] = AvailableRes[i] + RequestMat[user_defined_id][i];
            AllocationMat[user_defined_id][i] = AllocationMat[user_defined_id][i] - RequestMat[user_defined_id][i];
            NeedMat[user_defined_id][i] = NeedMat[user_defined_id][i] + RequestMat[user_defined_id][i];
        }

        pthread_cond_wait(&cond, &mutex); // Wait until a signal

        // Pretend to go into the new state again to confirm the new state is safe
        for (int i = 0; i < M; i++) {
            AvailableRes[i] = AvailableRes[i] - RequestMat[user_defined_id][i];
            AllocationMat[user_defined_id][i] = AllocationMat[user_defined_id][i] + RequestMat[user_defined_id][i];
            NeedMat[user_defined_id][i] = NeedMat[user_defined_id][i] - RequestMat[user_defined_id][i];
        }
    }

    // If we are here then it is safe to go to next state
    for (int i = 0; i < M; i++) {
        RequestMat[user_defined_id][i] = 0; // Request is completed
    }

    /* critical section end */
    pthread_mutex_unlock(&mutex);
    return 0;
}


int rm_release (int release[])
{
    /* critical section start */
	pthread_mutex_lock(&mutex);

    // Find the user defined id of the calling thread
    int user_defined_id = -1;
    for (int i = 0; i < N; i++) {
        if (threadList[i] == pthread_self()) {
            user_defined_id = i;
        }
    }

    // Conditions that the function has an error
    if (user_defined_id == -1) {
        /* critical section end */
	    pthread_mutex_unlock(&mutex);

        return -1;
    }

    // Return error if the released resources are more than the allocated ones
    for (int i = 0; i < M; i++) {
        if (release[i] > AllocationMat[user_defined_id][i]) {
            /* critical section end */
	        pthread_mutex_unlock(&mutex);

            return -1;
        }
    }

    // Release the resources
    for (int i = 0; i < M; i++) {
        AllocationMat[user_defined_id][i] = AllocationMat[user_defined_id][i] - release[i];
        AvailableRes[i] = AvailableRes[i] + release[i];

        if (DA == 1) {
            NeedMat[user_defined_id][i] = NeedMat[user_defined_id][i] + release[i];
        }
    }
    pthread_cond_broadcast(&cond);

    /* critical section end */
	pthread_mutex_unlock(&mutex);

    return 0;
}


int rm_detection()
{
    /* Critical section starts here */
    pthread_mutex_lock(&mutex);

    int Work[MAXR];
    int FinishTemp[MAXP];
    int isAllSmallerOrEqual = 1;
    int isDeadlocked = 0;

    // Create a work vector and initialize it with available vector
    for (int i = 0; i < M; i++) {
        Work[i] = AvailableRes[i];
    }

    // Populate the temporary finish function according to the requests of the threads
    for (int i = 0; i < N; i++) {
        FinishTemp[i] = 1;
        // If the thread is already finished by rm_thread_ended function then mark it as finished
        if (ThreadFinish[i] == 1) {
            continue;
        }

        for (int j = 0; j < M; j++) {
            if (RequestMat[i][j] != 0) {
                FinishTemp[i] = 0;
                break;
            }
        }
    }

    // The main detection
    int i = 0;
    while (1) {
        if (FinishTemp[i] == 0) {
            // Check if the request for each resource type is less than the available pool (Work)
            isAllSmallerOrEqual = 1;
            for (int j = 0; j < M; j++) {
                if (RequestMat[i][j] > Work[j]) {
                    isAllSmallerOrEqual = 0;
                    break;
                }
            }

            // if we find a thread that request less then or equal to the available resources
            if (isAllSmallerOrEqual == 1) {
                // update work vector
                for (int k = 0; k < M; k++) {
                    Work[k] = Work[k] + AllocationMat[i][k];
                }
                FinishTemp[i] = 1; // Mark the thread as finished
                i = -1; // start from the beginning (there is i++ at the end of the while loop so make it -1)
            }
        }

        // If we are at the last thread to check
        if (i == (N - 1)) {
            // Check if all threads are finished
            isDeadlocked = 0;
            for (int x = 0; x < N; x++) {
                if (FinishTemp[x] == 0) {
                    isDeadlocked = 1;
                    break;
                }
            }
            break;
        }

        i++;
    }
    // End of detection

    // If there is no deadlock
    if (isDeadlocked == 0) {
        /* critical section end */
	    pthread_mutex_unlock(&mutex);

        return 0;
    }

    // If there is deadlock
    else {
        // Count the number of deadlocked processes
        int countOfDeadlock = 0;
        for (int i = 0; i < N; i++) {
            if (FinishTemp[i] == 0) {
                countOfDeadlock++;
            }
        }

        /* critical section end */
	    pthread_mutex_unlock(&mutex);

        return countOfDeadlock;
    }
}


void rm_print_state (char hmsg[])
{
    /* critical section start */
	pthread_mutex_lock(&mutex);

    printf("#########################################\n");
    printf("%s\n", hmsg);
    printf("#########################################\n");

    printf("Exist:\n");
    printf("      ");
    for(int i = 0; i < M; i++) {
        printf("R%d   ", i);
    }
    printf("\n");
    for(int i = 0; i < M; i++) {
        if (i == 0) {
            printf("%7d", ExistingRes[i]);
        }
        else {
            printf("%5d", ExistingRes[i]);
        }
    }
    printf("\n\n");

    printf("Available:\n");
    printf("      ");
    for(int i = 0; i < M; i++) {
        printf("R%d   ", i);
    }
    printf("\n");
    for(int i = 0; i < M; i++) {
        if (i == 0) {
            printf("%7d", AvailableRes[i]);
        }
        else {
            printf("%5d", AvailableRes[i]);
        }
    }
    printf("\n\n");

    printf("Allocation:\n");
    printf("      ");
    for(int i = 0; i < M; i++) {
        printf("R%d   ", i);
    }
    printf("\n");
    for(int i = 0; i < N; i++) {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) {
            if (j == 0) {
                if (i / 10 == 0) {
                    printf("%3d", AllocationMat[i][j]);
                }
                else {
                    printf("%2d", AllocationMat[i][j]);
                }
            }
            else {
                printf("%5d", AllocationMat[i][j]);
            }
        }
        printf("\n");
    }
    printf("\n");

    printf("Request:\n");
    printf("      ");
    for(int i = 0; i < M; i++) {
        printf("R%d   ", i);
    }
    printf("\n");
    for(int i = 0; i < N; i++) {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) {
            if (j == 0) {
                if (i / 10 == 0) {
                    printf("%3d", RequestMat[i][j]);
                }
                else {
                    printf("%2d", RequestMat[i][j]);
                }
            }
            else {
                printf("%5d", RequestMat[i][j]);
            }
        }
        printf("\n");
    }
    printf("\n");

    printf("MaxDemand:\n");
    printf("      ");
    for(int i = 0; i < M; i++) {
        printf("R%d   ", i);
    }
    printf("\n");
    for(int i = 0; i < N; i++) {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) {
            if (j == 0) {
                if (i / 10 == 0) {
                    printf("%3d", MaxDemandMat[i][j]);
                }
                else {
                    printf("%2d", MaxDemandMat[i][j]);
                }
            }
            else {
                printf("%5d", MaxDemandMat[i][j]);
            }
        }
        printf("\n");
    }
    printf("\n");

    printf("Need:\n");
    printf("      ");
    for(int i = 0; i < M; i++) {
        printf("R%d   ", i);
    }
    printf("\n");
    for(int i = 0; i < N; i++) {
        printf("T%d: ", i);
        for (int j = 0; j < M; j++) {
            if (j == 0) {
                if (i / 10 == 0) {
                    printf("%3d", NeedMat[i][j]);
                }
                else {
                    printf("%2d", NeedMat[i][j]);
                }
            }
            else {
                printf("%5d", NeedMat[i][j]);
            }
        }
        printf("\n");
    }
    printf("#########################################\n\n");

    /* critical section end */
	pthread_mutex_unlock(&mutex);
}

// Additional Functions

// returns 1 if the system is currently safe, 0 if not safe, -1 if there is an error
int safety_check() {
    int Work[MAXR];
    int FinishTemp[MAXP];
    int isAllSmallerOrEqual = 1;

    // Create a work vector and initialize it with available vector
    for (int i = 0; i < M; i++) {
        Work[i] = AvailableRes[i];
    }

    for (int i = 0; i < N; i++) {
        if (ThreadFinish[i] == 1) {
            FinishTemp[i] = 1;
        }

        else {
            FinishTemp[i] = 0;
        }
    }

    // The main safety_check
    int i = 0;
    while (1) {
        if (FinishTemp[i] == 0) {
            // Check if the need for each resource type is less than the available pool (Work)
            isAllSmallerOrEqual = 1;
            for (int j = 0; j < M; j++) {
                if (NeedMat[i][j] > Work[j]) {
                    isAllSmallerOrEqual = 0;
                    break;
                }
            }

            // if we find a thread that need less then or equal to the available resources
            if (isAllSmallerOrEqual == 1) {
                // update work vector
                for (int k = 0; k < M; k++) {
                    Work[k] = Work[k] + AllocationMat[i][k];
                }
                FinishTemp[i] = 1; // Mark the thread as finished
                i = -1; // start from the beginning (there is i++ at the end of the while loop so make it -1)
            }
        }

        // If we are at the last thread to check
        if (i == (N - 1)) {
            for (int x = 0; x < N; x++) {
                if (FinishTemp[x] == 0) {
                    return 0;
                }
            }
            return 1;
        }

        i++;
    }
    // End of safety_check

    return -1; // If an error occured
}
