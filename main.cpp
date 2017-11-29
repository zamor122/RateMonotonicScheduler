#define GNU_SOURCE
#include <iostream>
#include <chrono>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

using namespace std;
// Rate Monotonic Scheduler


///////////////////////////////////
// Variables for program execution
///////////////////////////////////

pthread_attr_t attr0; // For main thread
pthread_attr_t attr1; // For T1 thread
pthread_attr_t attr2; // For T2 thread
pthread_attr_t attr3; // For T3 thread
pthread_attr_t attr4; // For T4 thread

int runAmntT1 = 1; // Thread 1 runs doWork() 1 time
int runAmntT2 = 2; // Thread 2 runs doWork() 2 times
int runAmntT3 = 4; // Thread 3 runs doWork() 4 times
int runAmntT4 = 16; // Thread 4 runs doWork() 16 times

// Counters for each thread
int counterT1;
int counterT2;
int counterT3;
int counterT4;

sched_param param0;
sched_param param1;
sched_param param2;
sched_param param3;
sched_param param4;

pthread_t schedulerThread;
pthread_t T1;
pthread_t T2;
pthread_t T3;
pthread_t T4;

//CPU
cpu_set_t cpu;

//DoWorkMatrix
int doWorkMatrix[10][10]; // 10x10 matrix for do work

/////////////////////////////////////////////////
// Struct: For passing in specific thread info
/////////////////////////////////////////////////
struct threadValues {
    int* runAmount;
    int* counter;
};

// Specific struct for this program;
threadValues tValArr[4]; // 4 Thread value structs. One for each of the four threads

/////////////////////////////
// Do Work Function
/////////////////////////////

void doWork() { // Busy work function, multiplies each column of a 10 x 10 matrix (all numbers are 1)
    int total = 1; // total of multiplication
    for(int i = 0; i < 4; i++) { // We go through 5 sets, with each set including 2 columns (the immediate column, then the column five to the right from it)
        for(int j = 0; j < 9; j++) {
            total*= doWorkMatrix[j][i]; // multiply all values in current column
        }
        for (int a = 0; a < 9; a++) {
            total *= doWorkMatrix[a][i+5]; // now multiply all the values in the column which is five columns over from the current one
        }
    }
    cout << "we are done" << endl;
}

//////////////////////////////////////
// Threading Function
//////////////////////////////////////

void *run_thread(void * param) {
    struct threadValues *passedInValues;
    passedInValues = (threadValues*) param;

    int runFor = *passedInValues->runAmount; // May be inefficient to create a local copy here
    for (int i = 0; i < runFor; i++) {
        doWork();
    }
    passedInValues->counter++; //Increment respective counter

    pthread_exit(nullptr);
}

//////////////////////////////////////
// Rate Monotonic Scheduler
//////////////////////////////////////

void *scheduler(void * param) {

    counterT1 = 0;
    counterT2 = 0;
    counterT3 = 0;
    counterT4 = 0;

    //Put in counters
    tValArr[0].counter = &counterT1;
    tValArr[1].counter = &counterT2;
    tValArr[2].counter = &counterT3;
    tValArr[3].counter = &counterT4;

    // Put in run time amount
    tValArr[0].runAmount = &runAmntT1;
    tValArr[1].runAmount = &runAmntT2;
    tValArr[2].runAmount = &runAmntT3;
    tValArr[3].runAmount = &runAmntT4;

    int tid1 = pthread_create(&T1, &attr1, run_thread, (void *) &tValArr[0]);
    int tid2 = pthread_create(&T2, &attr2, run_thread, (void *) &tValArr[1]);
    int tid3 = pthread_create(&T3, &attr3, run_thread, (void *) &tValArr[2]);
    int tid4 = pthread_create(&T4, &attr4, run_thread, (void *) &tValArr[3]);

    pthread_exit(nullptr);
}

int main() {

    // Initialize do work matrix
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            doWorkMatrix[i][j] = 1; // Set all matrix values to one
        }
    }
    // Set CPU priority to be the same for all threads;
    CPU_ZERO(&cpu);
    CPU_SET(2, &cpu); //  All threads should run on CPU 2

    pthread_attr_init(&attr0); // Initialize thread attributes
    pthread_attr_init(&attr1); // Initialize thread attributes
    pthread_attr_init(&attr2); // Initialize thread attributes
    pthread_attr_init(&attr3); // Initialize thread attributes
    pthread_attr_init(&attr4); // Initialize thread attributes

    pthread_attr_setschedparam(&attr0, &param0);  // Set thread priority
    pthread_attr_setschedparam(&attr1, &param0);  // Set thread priority
    pthread_attr_setschedparam(&attr2, &param0);  // Set thread priority
    pthread_attr_setschedparam(&attr3, &param0);  // Set thread priority
    pthread_attr_setschedparam(&attr4, &param0);  // Set thread priority

    pthread_setaffinity_np(schedulerThread, sizeof(&cpu), &cpu); // Set processor affinity;
    pthread_setaffinity_np(T1, sizeof(&cpu), &cpu); // Set processor affinity;
    pthread_setaffinity_np(T2, sizeof(&cpu), &cpu); // Set processor affinity;
    pthread_setaffinity_np(T3, sizeof(&cpu), &cpu); // Set processor affinity;
    pthread_setaffinity_np(T4, sizeof(&cpu), &cpu); // Set processor affinity;


    param0.__sched_priority = 10; // DOUBLE CHECK THIS IF YOU RUN INTO TROUBLE
    param1.__sched_priority = 9; // DOUBLE CHECK THIS IF YOU RUN INTO TROUBLE
    param2.__sched_priority = 8; // DOUBLE CHECK THIS IF YOU RUN INTO TROUBLE
    param3.__sched_priority = 7; // DOUBLE CHECK THIS IF YOU RUN INTO TROUBLE
    param4.__sched_priority = 6; // DOUBLE CHECK THIS IF YOU RUN INTO TROUBLE

    //Temp var
    int tempParam = 0;

    int tidSchThr = pthread_create(&schedulerThread, &attr0, scheduler, &tempParam);

    /*
    // Now test ms clock values with Chrono
    auto time1 = chrono::high_resolution_clock::now();
    // Let's test a bunch
    //for (int test = 0; test < 10000; test++)
        doWork();
    auto time2 = chrono::high_resolution_clock::now();
    // Convert to MS (Whole Milliseconds) with duration cast
    auto wms_conversion = chrono::duration_cast<chrono::milliseconds>(time2 - time1);
    // Convert to Fractional MS with duration
    chrono::duration<double, milli> fms_conversion = (time2 - time1);

    cout << wms_conversion.count() << " whole seconds" << endl;
    cout << fms_conversion.count() << " milliseconds" << endl; */

    return 0;
}
