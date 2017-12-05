#include <sched.h>
#include <iostream>
#include <chrono>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <atomic>
#include <time.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <mutex>
using namespace std;


//doWork method
int doWorkMatrix[10][10];
struct Values {
    long* runAmount;
    int* counter;
    sem_t* semaphore;
    atomic<bool>* finished;
};
Values arrayValues[4];

void doWork() {
    int total = 1;
    for(int i = 0; i < 4; i++) {
        for (int j = 0; j < 9; j++) {
            total *= doWorkMatrix[j][i];
        }
        for (int k = 0; k < 9; k++) {
            total *= doWorkMatrix[k][i + 5];
        }
    }
}


//variables
long runAmntT0 = 1;
long runAmntT1 = 2;
long runAmntT2 = 4;
long runAmntT3 = 16;
int periodT0 = 1;
int periodT1 = 2;
int periodT2 = 4;
int periodT3 = 16;
atomic<bool> finishedMain(false);
atomic<bool> finishedT1(false);
atomic<bool> finishedT2(false);
atomic<bool> finishedT3(false);
int frame = 16;
int programRuntime = 10;
int conversionTimeFactor = 1000000;
int timeUnit = 10;
int timeCounter = 0;
bool finished = false;
pthread_attr_t attribute0; // main
pthread_attr_t attribute1; // mainThread
pthread_attr_t attribute2; // thread1
pthread_attr_t attribute3; // thread2
pthread_attr_t attribute4; // thread3
//thread counters
int countMain;
int countThread1;
int countThread2;
int countThread3;
//count missed deadlines
int deadlinesMissedMain;
int deadlinesMissedThread1;
int deadlinesMissedThread2;
int deadlinesMissedThread3;

sched_param mainParamater;
sched_param thread1Parameter;
sched_param thread2Parameter;
sched_param thread3Parameter;
sched_param thread4Parameter;
pthread_t schedulingThread;

pthread_t mainThread = 0;
pthread_t thread1 = 0;
pthread_t thread2 = 0;
pthread_t thread3 = 0;

cpu_set_t cpu;

sem_t schedulerSemaphore;
sem_t syncMain;
sem_t synThread1;
sem_t syncThread2;
sem_t syncThread3;


//function to unlock thread and increase time counter
void unlockSempahore(int sig, siginfo_t *si, void *uc)
{
    timeCounter++;
    sem_post(&schedulerSemaphore);
}

//function to run the thread
void *runThread(void *param) {
    while(!finished) {
        sem_wait(((Values*)param)->semaphore);

        *((Values*)param)->finished = false;
        for (int i = 0; i < *((Values*)param)->runAmount; i++) {
            doWork();
        }
        *((Values*)param)->finished = true;
        *((Values*)param)->counter += 1;

    }
    pthread_exit(nullptr);
}
//check for overruns
bool checkForOverruns(bool *firstRunBool) {
    if (!*firstRunBool && !(*arrayValues[0].finished).load())
        deadlinesMissedMain++;
    if (!*firstRunBool && !(*arrayValues[1].finished).load())
        deadlinesMissedThread1++;
    if (!*firstRunBool && !(*arrayValues[2].finished).load())
        deadlinesMissedThread2++;
    if (!*firstRunBool && !(*arrayValues[3].finished).load())
        deadlinesMissedThread3++;
}

//main scheduler thread
void *scheduler(void * param) {
    //check to see if it's the first time running through
    bool firstTime = true;

    for (int schedulerPeriod = 0; schedulerPeriod < programRuntime; schedulerPeriod++) {
        for (int periodTime = 0; periodTime < frame; periodTime++) {
            sem_wait(&schedulerSemaphore);
            //run full 16
            if (!firstTime && !(*arrayValues[0].finished).load())
                deadlinesMissedMain++;
            sem_post(&syncMain);
            //run 4
            if (periodTime == 0 || periodTime == 2 || periodTime == 4 || periodTime == 6 || periodTime == 8 ||
                periodTime == 10 || periodTime == 12 || periodTime == 14) {
                if (!firstTime && !(*arrayValues[1].finished).load())
                    deadlinesMissedThread1++;
                sem_post(&synThread1);
            }
            //run 2
            if (periodTime == 0 || periodTime == 4 || periodTime == 8 || periodTime == 12) {

                if (!firstTime && !(*arrayValues[2].finished).load())
                    deadlinesMissedThread2++;
                sem_post(&syncThread2);
            }
            //run 1
            if (periodTime == 0) {
                if (!firstTime && !(*arrayValues[3].finished).load())
                    deadlinesMissedThread3++;
                sem_post(&syncThread3);
            }
            //no overruns
            if(firstTime)
                firstTime = false;
        }
    }
    //double check
    checkForOverruns(&firstTime);
    //exit
    pthread_exit(nullptr);
}

//run full program without overruns
void runNormal(){
    //doWork
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            doWorkMatrix[i][j] = 1;
        }
    }
    //initialize all variables
    sem_init(&schedulerSemaphore, 1, 0);
    sem_init(&syncMain, 1, 0);
    sem_init(&synThread1, 1, 0);
    sem_init(&syncThread2, 1, 0);
    sem_init(&syncThread3, 1, 0);
    countMain = 0;//counters
    countThread1 = 0;
    countThread2 = 0;
    countThread3 = 0;
    deadlinesMissedMain = 0;
    deadlinesMissedThread1 = 0;
    deadlinesMissedThread2 = 0;
    deadlinesMissedThread3 = 0;
    CPU_ZERO(&cpu);
    CPU_SET(0, &cpu);
    pthread_attr_init(&attribute0);
    pthread_attr_init(&attribute1);
    pthread_attr_init(&attribute2);
    pthread_attr_init(&attribute3);
    pthread_attr_init(&attribute4);
    pthread_attr_setaffinity_np(&attribute0, sizeof(cpu_set_t), &cpu);
    pthread_attr_setaffinity_np(&attribute1, sizeof(cpu_set_t), &cpu);
    pthread_attr_setaffinity_np(&attribute2, sizeof(cpu_set_t), &cpu);
    pthread_attr_setaffinity_np(&attribute3, sizeof(cpu_set_t), &cpu);
    pthread_attr_setaffinity_np(&attribute4, sizeof(cpu_set_t), &cpu);
    mainParamater.__sched_priority = sched_get_priority_max(SCHED_FIFO);
    thread1Parameter.__sched_priority = 90;
    thread2Parameter.__sched_priority = 80;
    thread3Parameter.__sched_priority = 70;
    thread4Parameter.__sched_priority = 60;
    pthread_attr_setschedpolicy(&attribute0, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attribute1, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attribute2, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attribute3, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attribute4, SCHED_FIFO);
    pthread_attr_setschedparam(&attribute0, &mainParamater);
    pthread_attr_setschedparam(&attribute1, &thread1Parameter);
    pthread_attr_setschedparam(&attribute2, &thread2Parameter);
    pthread_attr_setschedparam(&attribute3, &thread3Parameter);
    pthread_attr_setschedparam(&attribute4, &thread4Parameter);
    pthread_attr_setinheritsched(&attribute0, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attribute1, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attribute2, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attribute3, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attribute4, PTHREAD_EXPLICIT_SCHED);
    arrayValues[0].counter = &countMain;
    arrayValues[1].counter = &countThread1;
    arrayValues[2].counter = &countThread2;
    arrayValues[3].counter = &countThread3;
    arrayValues[0].runAmount = &runAmntT0;
    arrayValues[1].runAmount = &runAmntT1;
    arrayValues[2].runAmount = &runAmntT2;
    arrayValues[3].runAmount = &runAmntT3;
    arrayValues[0].semaphore = &syncMain;
    arrayValues[1].semaphore = &synThread1;
    arrayValues[2].semaphore = &syncThread2;
    arrayValues[3].semaphore = &syncThread3;
    arrayValues[0].finished = &finishedMain;
    arrayValues[1].finished = &finishedT1;
    arrayValues[2].finished = &finishedT2;
    arrayValues[3].finished = &finishedT3;
    pthread_create(&mainThread, &attribute1, runThread, (void *) &arrayValues[0]);
    pthread_create(&thread1, &attribute2, runThread, (void *) &arrayValues[1]);
    pthread_create(&thread2, &attribute3, runThread, (void *) &arrayValues[2]);
    pthread_create(&thread3, &attribute4, runThread, (void *) &arrayValues[3]);
    int tidSchThr = pthread_create(&schedulingThread, &attribute0, scheduler, nullptr);
    long oneUnitOfTime = conversionTimeFactor*timeUnit;
    long totalRuntime = oneUnitOfTime*programRuntime*conversionTimeFactor; // 1 unit(10 ms) x 16 unit frame period  x 10 unit program period
    struct sigevent sig;
    struct sigaction sa;
    int sigNo = SIGRTMIN;
    timer_t unitTimer;
    itimerspec its;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = unlockSempahore;
    sigemptyset(&sa.sa_mask);
    sigaction(sigNo, &sa, nullptr);
    sig.sigev_notify = SIGEV_SIGNAL;
    sig.sigev_signo = sigNo;
    sig.sigev_value.sival_ptr = nullptr;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = oneUnitOfTime; // Expiration time  oneUnitOfTime
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = its.it_value.tv_nsec; // Same as above and it repeats  its.it_value.tv_nsec
    //create timer
    timer_create(CLOCK_REALTIME, &sig, &unitTimer);
    if (timer_settime(unitTimer, 0, &its, nullptr) == -1) // Start Timer
        cout << "FAILURE" << endl;

    //end program
    pthread_join(schedulingThread, nullptr);
    finished = true;
    cout << "Main Ran " << countMain << " Times" << endl;
    cout << "Thread 1 Ran " << countThread1 << " Times" << endl;
    cout << "Thread 2 Ran " << countThread2 << " Times" << endl;
    cout << "Thread 3 Ran " << countThread3 << " Times\n" << endl;

    // Print out results
    cout << "Main had " << deadlinesMissedMain << " missed deadlines" << endl;
    cout << "Thread 1 had " << deadlinesMissedThread1 << " missed deadlines" << endl;
    cout << "Thread 2 had " << deadlinesMissedThread2 << " missed deadlines" << endl;
    cout << "Thread 3 had " << deadlinesMissedThread3 << " missed deadlines" << endl;
}

int runOverrunThread1(){
    runAmntT1 = 200000000000000000;
    //doWork
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            doWorkMatrix[i][j] = 1;
        }
    }
    //initialize all variables
    sem_init(&schedulerSemaphore, 1, 0);
    sem_init(&syncMain, 1, 0);
    sem_init(&synThread1, 1, 0);
    sem_init(&syncThread2, 1, 0);
    sem_init(&syncThread3, 1, 0);
    countMain = 0;//counters
    countThread1 = 0;
    countThread2 = 0;
    countThread3 = 0;
    deadlinesMissedMain = 0;
    deadlinesMissedThread1 = 0;
    deadlinesMissedThread2 = 0;
    deadlinesMissedThread3 = 0;
    CPU_ZERO(&cpu);
    CPU_SET(0, &cpu);
    pthread_attr_init(&attribute0);
    pthread_attr_init(&attribute1);
    pthread_attr_init(&attribute2);
    pthread_attr_init(&attribute3);
    pthread_attr_init(&attribute4);
    pthread_attr_setaffinity_np(&attribute0, sizeof(cpu_set_t), &cpu);
    pthread_attr_setaffinity_np(&attribute1, sizeof(cpu_set_t), &cpu);
    pthread_attr_setaffinity_np(&attribute2, sizeof(cpu_set_t), &cpu);
    pthread_attr_setaffinity_np(&attribute3, sizeof(cpu_set_t), &cpu);
    pthread_attr_setaffinity_np(&attribute4, sizeof(cpu_set_t), &cpu);
    mainParamater.__sched_priority = sched_get_priority_max(SCHED_FIFO);
    thread1Parameter.__sched_priority = 90;
    thread2Parameter.__sched_priority = 80;
    thread3Parameter.__sched_priority = 70;
    thread4Parameter.__sched_priority = 60;
    pthread_attr_setschedpolicy(&attribute0, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attribute1, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attribute2, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attribute3, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attribute4, SCHED_FIFO);
    pthread_attr_setschedparam(&attribute0, &mainParamater);
    pthread_attr_setschedparam(&attribute1, &thread1Parameter);
    pthread_attr_setschedparam(&attribute2, &thread2Parameter);
    pthread_attr_setschedparam(&attribute3, &thread3Parameter);
    pthread_attr_setschedparam(&attribute4, &thread4Parameter);
    pthread_attr_setinheritsched(&attribute0, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attribute1, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attribute2, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attribute3, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attribute4, PTHREAD_EXPLICIT_SCHED);
    arrayValues[0].counter = &countMain;
    arrayValues[1].counter = &countThread1;
    arrayValues[2].counter = &countThread2;
    arrayValues[3].counter = &countThread3;
    arrayValues[0].runAmount = &runAmntT0;
    arrayValues[1].runAmount = &runAmntT1;
    arrayValues[2].runAmount = &runAmntT2;
    arrayValues[3].runAmount = &runAmntT3;
    arrayValues[0].semaphore = &syncMain;
    arrayValues[1].semaphore = &synThread1;
    arrayValues[2].semaphore = &syncThread2;
    arrayValues[3].semaphore = &syncThread3;
    arrayValues[0].finished = &finishedMain;
    arrayValues[1].finished = &finishedT1;
    arrayValues[2].finished = &finishedT2;
    arrayValues[3].finished = &finishedT3;
    pthread_create(&mainThread, &attribute1, runThread, (void *) &arrayValues[0]);
    pthread_create(&thread1, &attribute2, runThread, (void *) &arrayValues[1]);
    pthread_create(&thread2, &attribute3, runThread, (void *) &arrayValues[2]);
    pthread_create(&thread3, &attribute4, runThread, (void *) &arrayValues[3]);
    int tidSchThr = pthread_create(&schedulingThread, &attribute0, scheduler, nullptr);
    long oneUnitOfTime = conversionTimeFactor*timeUnit;
    long totalRuntime = oneUnitOfTime*programRuntime*conversionTimeFactor;
    struct sigevent sig;
    struct sigaction sa;
    int sigNo = SIGRTMIN;
    timer_t unitTimer;
    itimerspec its;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = unlockSempahore;
    sigemptyset(&sa.sa_mask);
    sigaction(sigNo, &sa, nullptr);
    sig.sigev_notify = SIGEV_SIGNAL;
    sig.sigev_signo = sigNo;
    sig.sigev_value.sival_ptr = nullptr;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = oneUnitOfTime;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    //create timer
    timer_create(CLOCK_REALTIME, &sig, &unitTimer);
    if (timer_settime(unitTimer, 0, &its, nullptr) == -1)
        cout << "FAILURE" << endl;

    //end program
    pthread_join(schedulingThread, nullptr);
    finished = true;
    cout << "Main Ran " << countMain << " Times" << endl;
    cout << "Thread 1 Ran " << countThread1 << " Times" << endl;
    cout << "Thread 2 Ran " << countThread2 << " Times" << endl;
    cout << "Thread 3 Ran " << countThread3 << " Times\n" << endl;

    // Print out results
    cout << "Main had " << deadlinesMissedMain << " missed deadlines" << endl;
    cout << "Thread 1 had " << deadlinesMissedThread1 << " missed deadlines" << endl;
    cout << "Thread 2 had " << deadlinesMissedThread2 << " missed deadlines" << endl;
    cout << "Thread 3 had " << deadlinesMissedThread3 << " missed deadlines" << endl;

    return 0;
}

int runOverrunThread2(){
    runAmntT2 = 4000000000000000000;
    //doWork
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            doWorkMatrix[i][j] = 1;
        }
    }
    //initialize all variables
    sem_init(&schedulerSemaphore, 1, 0);
    sem_init(&syncMain, 1, 0);
    sem_init(&synThread1, 1, 0);
    sem_init(&syncThread2, 1, 0);
    sem_init(&syncThread3, 1, 0);
    countMain = 0;//counters
    countThread1 = 0;
    countThread2 = 0;
    countThread3 = 0;
    deadlinesMissedMain = 0;
    deadlinesMissedThread1 = 0;
    deadlinesMissedThread2 = 0;
    deadlinesMissedThread3 = 0;
    CPU_ZERO(&cpu);
    CPU_SET(0, &cpu);
    pthread_attr_init(&attribute0);
    pthread_attr_init(&attribute1);
    pthread_attr_init(&attribute2);
    pthread_attr_init(&attribute3);
    pthread_attr_init(&attribute4);
    pthread_attr_setaffinity_np(&attribute0, sizeof(cpu_set_t), &cpu);
    pthread_attr_setaffinity_np(&attribute1, sizeof(cpu_set_t), &cpu);
    pthread_attr_setaffinity_np(&attribute2, sizeof(cpu_set_t), &cpu);
    pthread_attr_setaffinity_np(&attribute3, sizeof(cpu_set_t), &cpu);
    pthread_attr_setaffinity_np(&attribute4, sizeof(cpu_set_t), &cpu);
    mainParamater.__sched_priority = sched_get_priority_max(SCHED_FIFO);
    thread1Parameter.__sched_priority = 90;
    thread2Parameter.__sched_priority = 80;
    thread3Parameter.__sched_priority = 70;
    thread4Parameter.__sched_priority = 60;
    pthread_attr_setschedpolicy(&attribute0, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attribute1, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attribute2, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attribute3, SCHED_FIFO);
    pthread_attr_setschedpolicy(&attribute4, SCHED_FIFO);
    pthread_attr_setschedparam(&attribute0, &mainParamater);
    pthread_attr_setschedparam(&attribute1, &thread1Parameter);
    pthread_attr_setschedparam(&attribute2, &thread2Parameter);
    pthread_attr_setschedparam(&attribute3, &thread3Parameter);
    pthread_attr_setschedparam(&attribute4, &thread4Parameter);
    pthread_attr_setinheritsched(&attribute0, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attribute1, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attribute2, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attribute3, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&attribute4, PTHREAD_EXPLICIT_SCHED);
    arrayValues[0].counter = &countMain;
    arrayValues[1].counter = &countThread1;
    arrayValues[2].counter = &countThread2;
    arrayValues[3].counter = &countThread3;
    arrayValues[0].runAmount = &runAmntT0;
    arrayValues[1].runAmount = &runAmntT1;
    arrayValues[2].runAmount = &runAmntT2;
    arrayValues[3].runAmount = &runAmntT3;
    arrayValues[0].semaphore = &syncMain;
    arrayValues[1].semaphore = &synThread1;
    arrayValues[2].semaphore = &syncThread2;
    arrayValues[3].semaphore = &syncThread3;
    arrayValues[0].finished = &finishedMain;
    arrayValues[1].finished = &finishedT1;
    arrayValues[2].finished = &finishedT2;
    arrayValues[3].finished = &finishedT3;
    pthread_create(&mainThread, &attribute1, runThread, (void *) &arrayValues[0]);
    pthread_create(&thread1, &attribute2, runThread, (void *) &arrayValues[1]);
    pthread_create(&thread2, &attribute3, runThread, (void *) &arrayValues[2]);
    pthread_create(&thread3, &attribute4, runThread, (void *) &arrayValues[3]);
    int tidSchThr = pthread_create(&schedulingThread, &attribute0, scheduler, nullptr);
    long oneUnitOfTime = conversionTimeFactor*timeUnit;
    long totalRuntime = oneUnitOfTime*programRuntime*conversionTimeFactor;
    struct sigevent sig;
    struct sigaction sa;
    int sigNo = SIGRTMIN;
    timer_t unitTimer;
    itimerspec its;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = unlockSempahore;
    sigemptyset(&sa.sa_mask);
    sigaction(sigNo, &sa, nullptr);
    sig.sigev_notify = SIGEV_SIGNAL;
    sig.sigev_signo = sigNo;
    sig.sigev_value.sival_ptr = nullptr;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = oneUnitOfTime;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    //create timer
    timer_create(CLOCK_REALTIME, &sig, &unitTimer);
    if (timer_settime(unitTimer, 0, &its, nullptr) == -1)
        cout << "FAILURE" << endl;

    //end program
    pthread_join(schedulingThread, nullptr);
    finished = true;
    cout << "Main Ran " << countMain << " Times" << endl;
    cout << "Thread 1 Ran " << countThread1 << " Times" << endl;
    cout << "Thread 2 Ran " << countThread2 << " Times" << endl;
    cout << "Thread 3 Ran " << countThread3 << " Times\n" << endl;

    // Print out results
    cout << "Main had " << deadlinesMissedMain << " missed deadlines" << endl;
    cout << "Thread 1 had " << deadlinesMissedThread1 << " missed deadlines" << endl;
    cout << "Thread 2 had " << deadlinesMissedThread2 << " missed deadlines" << endl;
    cout << "Thread 3 had " << deadlinesMissedThread3 << " missed deadlines" << endl;

    return 0;
}


int main() {
    cout << "Enter 1 for normal run through." << endl;
    cout << "Enter 2 for overrun option 1" << endl;
    cout << "Enter 3 for overrun option 2" << endl;
    cout << "Enter 0 to exit application.\n" << endl;
    int option;
    cin >> option;
    while(option != 0 || option != 1 || option != 2 || option != 3){
        // Check test case
        if (option == 1) {
            runNormal();
            break;
        } else if (option == 2) {
            runOverrunThread1();
            break;
        } else if (option == 3) {
            runOverrunThread2();
            break;
        }else if (option == 0){
            exit(0);
        }
        else {
            cout << "Enter a valid option: " << endl;
            cin >> option;
        }
    }
    return 0;
}
