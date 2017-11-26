#include <iostream>
#include <chrono>


using namespace std;
// Rate Monotonic Scheduler

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

    // Initialize do work matrix
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            doWorkMatrix[i][j] = 1; // Set all matrix values to one
        }
    }

    // Now test ms clock values with Chrono
    auto time1 = chrono::high_resolution_clock::now();
    // Let's test a bunch
    for (int test = 0; test < 1000; test++)
        doWork();
    auto time2 = chrono::high_resolution_clock::now();
    // Convert to MS (Whole Milliseconds) with duration cast
    auto wms_conversion = chrono::duration_cast<chrono::milliseconds>(time2 - time1);
    // Convert to Fractional MS with duration
    chrono::duration<double, milli> fms_conversion = (time2 - time1);

    cout << wms_conversion.count() << endl;
    cout << fms_conversion.count() << endl;

    return 0;
}
