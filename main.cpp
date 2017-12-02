#include <sched.h>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <semaphore.h>
#include <sys/sysinfo.h>
using namespace std;
//doWork function to do busy work
int busyMatrix[10][10];
struct values {
    int* multiplier;
    int* counter;
    sem_t* semaphore;
};
//values used
values tValArr[4];
//main doWork function
void doWork() {
    int total = 1;
    for(int i = 0; i < 4; i++) {
        for (int j = 0; j < 9; j++) {
            total *= busyMatrix[j][i];
        }
        for (int k = 0; k < 9; k++) {
            total *= busyMatrix[k][i + 5];
        }
    }
}
//begin rate monotonic rms
//thread attributes
pthread_attr_t attribute0;
pthread_attr_t attribute1;
pthread_attr_t attribute2;
pthread_attr_t attribute3;
pthread_attr_t attribute4;
//run times
int run1 = 1;
int run2 = 2;
int run4 = 4;
int run16 = 16;
//frame declaration
int frame = 16;
// Counters
int counter1;
int counter2;
int counter3;
int counter4;
// Parameters for scheduling
sched_param param0;
sched_param param1;
sched_param param2;
sched_param param3;
sched_param param4;
// Pthreads
pthread_t mainThread;
pthread_t thread1;
pthread_t thread2;
pthread_t thread3;
pthread_t thread4;
//CPU
cpu_set_t cpu;
// Semaphore declaration
sem_t sem1;
sem_t sem2;
sem_t sem3;
sem_t sem4;
void *run_thread(void * param) {
    struct values *args;
    args = (values*) param;
    sem_wait(args->semaphore);
    for (int i = 0; i < *args->multiplier; i++) {
        doWork();
        *args->counter += 1;
    }
    sleep(1);
    if (*args->multiplier == 1)
        cout << "Thread 1 Working..." << endl;
    if (*args->multiplier == 2)
        cout << "Thread 2 Working..." << endl;
    if (*args->multiplier == 4)
        cout << "Thread 3 Working..." << endl;
    if (*args->multiplier == 16)
        cout << "Thread 4 Working..." << endl;
    pthread_exit(nullptr);
}
void *rms(void *param) {
    auto time1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < frame; i++) {
        int t1 = pthread_create(&thread1, &attribute1, run_thread, (void *) &tValArr[0]);
        int t2 = pthread_create(&thread2, &attribute2, run_thread, (void *) &tValArr[1]);
        int t3 = pthread_create(&thread3, &attribute3, run_thread, (void *) &tValArr[2]);
        int t4 = pthread_create(&thread4, &attribute4, run_thread, (void *) &tValArr[3]);
        sem_post(&sem1);
        sem_post(&sem2);
        sem_post(&sem3);
        sem_post(&sem4);
        // Join threads
        pthread_join(thread1, nullptr);
        pthread_join(thread2, nullptr);
        pthread_join(thread3, nullptr);
        pthread_join(thread4, nullptr);
    }
    auto time2 = chrono::high_resolution_clock::now();
    auto conversion1 = chrono::duration_cast<chrono::milliseconds>(time2 - time1);
    chrono::duration<double, milli> conversion2 = (time2 - time1);
    cout << conversion1.count() << " s" << endl;
    cout << conversion2.count() << " ms" << endl;
    pthread_exit(nullptr);
}
//main
int main() {
cout << "Rate Monotonic Scheduler Starting" << endl;
//initialize everything
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            busyMatrix[i][j] = 1;
        }
    }
    sem_init(&sem1, 1, 1);    //semaphores
    sem_init(&sem2, 1, 1);
    sem_init(&sem3, 1, 1);
    sem_init(&sem4, 1, 1);
    counter1 = 0;    //counters
    counter2 = 0;
    counter3 = 0;
    counter4 = 0;
    CPU_ZERO(&cpu);    //set affinity
    CPU_SET(0, &cpu);
    pthread_attr_setaffinity_np(&attribute0, sizeof(cpu_set_t), &cpu); // Set processor affinity;
    pthread_attr_setaffinity_np(&attribute1, sizeof(cpu_set_t), &cpu); // Set processor affinity;
    pthread_attr_setaffinity_np(&attribute2, sizeof(cpu_set_t), &cpu); // Set processor affinity;
    pthread_attr_setaffinity_np(&attribute3, sizeof(cpu_set_t), &cpu); // Set processor affinity;
    pthread_attr_setaffinity_np(&attribute4, sizeof(cpu_set_t), &cpu); // Set processor affinity;
    pthread_attr_init(&attribute0);    //attributes
    pthread_attr_init(&attribute1);
    pthread_attr_init(&attribute2);
    pthread_attr_init(&attribute3);
    pthread_attr_init(&attribute4);
    param0.__sched_priority = 110; //priorities
    param1.__sched_priority = 109;
    param2.__sched_priority = 108;
    param3.__sched_priority = 107;
    param4.__sched_priority = 106;
    pthread_attr_setschedparam(&attribute0, &param0);
    pthread_attr_setschedparam(&attribute1, &param1);
    pthread_attr_setschedparam(&attribute2, &param2);
    pthread_attr_setschedparam(&attribute3, &param3);
    pthread_attr_setschedparam(&attribute4, &param4);
    tValArr[0].counter = &counter1; //counters
    tValArr[1].counter = &counter2;
    tValArr[2].counter = &counter3;
    tValArr[3].counter = &counter4;
    tValArr[0].multiplier = &run1; //runtimes
    tValArr[1].multiplier = &run2;
    tValArr[2].multiplier = &run4;
    tValArr[3].multiplier = &run16;
    tValArr[0].semaphore = &sem1; //semaphores
    tValArr[1].semaphore = &sem2;
    tValArr[2].semaphore = &sem3;
    tValArr[3].semaphore = &sem4;
    int tempParam = 0;
    int tidSchThr = pthread_create(&mainThread, &attribute0, rms, &tempParam);
    pthread_join(mainThread, nullptr);
    //end
    cout << "Scheduler ending, results displayed below: " << endl;
    cout << "Count for Thread 1: " << counter1 << endl;
    cout << "Count for Thread 2: " << counter2 << endl;
    cout << "Count for Thread 3: " << counter3 << endl;
    cout << "Count for Thread 4: " << counter4 << endl;
    auto time1 = chrono::high_resolution_clock::now();
    doWork();
    auto time2 = chrono::high_resolution_clock::now();
    auto wms_conversion = chrono::duration_cast<chrono::milliseconds>(time2 - time1);
    chrono::duration<double, milli> fms_conversion = (time2 - time1);
    cout << wms_conversion.count() << " s" << endl;
    cout << fms_conversion.count() << " ms" << endl;
    return 0;
}
