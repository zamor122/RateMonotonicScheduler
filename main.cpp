#define _GNU_SOURCE
#include <iostream>
#include <chrono>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

using namespace std;
// Rate Monotonic Scheduler

// Basic Attribute for threads. Set later to have different priorities (make a copy of the value)

// 5 Pthreads
pthread_attr_t attr0; // For main thread
pthread_attr_t attr1; // For T1 thread
pthread_attr_t attr2; // For T2 thread
pthread_attr_t attr3; // For T3 thread
pthread_attr_t attr4; // For T4 thread

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


int doWorkMatrix[10][10]; // 10x10 matrix for do work

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

int main() {
    // Set CPU priority to be the same for all threads;
    CPU_ZERO(&cpu);
    CPU_SET(2, &cpu); //  All threads should run on CPU 2

    pthread_attr_init(&attr0);
    pthread_attr_init(&attr1);
    pthread_attr_init(&attr2);
    pthread_attr_init(&attr3);
    pthread_attr_init(&attr4);

    pthread_attr_setschedparam(&attr0, &param0);  // Set thread priority
    pthread_attr_setschedparam(&attr1, &param0);  // Set thread priority
    pthread_attr_setschedparam(&attr2, &param0);  // Set thread priority
    pthread_attr_setschedparam(&attr3, &param0);  // Set thread priority
    pthread_attr_setschedparam(&attr4, &param0);  // Set thread priority

    pthread_setaffinity_np(schedulerThread, sizeof(&cpu), &cpu);
    pthread_setaffinity_np(T1, sizeof(&cpu), &cpu);
    pthread_setaffinity_np(T2, sizeof(&cpu), &cpu);
    pthread_setaffinity_np(T3, sizeof(&cpu), &cpu);
    pthread_setaffinity_np(T4, sizeof(&cpu), &cpu);


    param0.__sched_priority = 10; // DOUBLE CHECK THIS IF YOU RUN INTO TROUBLE
    param1.__sched_priority = 9; // DOUBLE CHECK THIS IF YOU RUN INTO TROUBLE
    param2.__sched_priority = 8; // DOUBLE CHECK THIS IF YOU RUN INTO TROUBLE
    param3.__sched_priority = 7; // DOUBLE CHECK THIS IF YOU RUN INTO TROUBLE
    param4.__sched_priority = 6; // DOUBLE CHECK THIS IF YOU RUN INTO TROUBLE


    schedulerThread = pthread_create(&schedulerThread, &attr0, PASS IN YOUR SCHEDULER FUNCTION HERE, PASS IN THE NEEDED ARGS);
    schedulerThread = pthread_create(&T1, &attr1, PASS IN YOUR SCHEDULER FUNCTION HERE, PASS IN THE NEEDED ARGS);
    schedulerThread = pthread_create(&T2, &attr2, PASS IN YOUR SCHEDULER FUNCTION HERE, PASS IN THE NEEDED ARGS);
    schedulerThread = pthread_create(&T3, &attr3, PASS IN YOUR SCHEDULER FUNCTION HERE, PASS IN THE NEEDED ARGS);
    schedulerThread = pthread_create(&T4, &attr4, PASS IN YOUR SCHEDULER FUNCTION HERE, PASS IN THE NEEDED ARGS);


    // Initialize do work matrix
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            doWorkMatrix[i][j] = 1; // Set all matrix values to one
        }
    }



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
    cout << fms_conversion.count() << " milliseconds" << endl;

    return 0;
}
