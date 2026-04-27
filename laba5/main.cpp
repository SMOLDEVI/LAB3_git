#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <string>
#include <iomanip>

using namespace std;

bool binarySearch1(int* A, int n, int key) {
    int left = 0;
    int right = n;
    while (right - left > 1) {
        int middle = (left + right) / 2;
        if (A[middle] > key)
            right = middle;
        else
            left = middle;
    }
    return A[left] == key;
}

bool binarySearch2(int* A, int n, int key) {
    int left = 0;
    int right = n;
    while (right - left > 1) {
        int middle = (left + right) / 2;
        if (A[middle] == key)
            return true;
        if (A[middle] > key)
            right = middle;
        else
            left = middle;
    }
    return A[left] == key;
}

bool binarySearch3(int* A, int n, int key) {
    int left = 0;
    int right = n;
    while (right - left > 1) {
        int middle = (left + right) / 2;
        if (A[middle] > key)
            right = middle;
        else {
            if (A[middle] == key)
                return true;
            left = middle;
        }
    }
    return A[left] == key;
}

void sf(int* arr, int size) {
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

void exportToExcel(double results[3][3][10], double averages[3][3]) {

    ofstream out("report.csv");
    if (!out.is_open()) {
        cerr << "Не удалось создать report.csv" << endl;
        return;
    }

    out << "\xEF\xBB\xBF";
    out << "sep=;" << endl;

    string testNames[3] = {
        "100%",
        "50%",
        "0%"
    };
    string algNames[3] = {
        "Алг 1",
        "Алг 2",
        "Алг 3"
    };

    for (int test = 0; test < 3; test++) {
        for (int alg = 0; alg < 3; alg++) {
            out << testNames[test] << " — " << algNames[alg] << endl;
            out << "Прогон";
            for (int run = 0; run < 10; run++)
                out << ";" << run + 1;
            out << ";Среднее" << endl;

            out << "Время (мс)";
            for (int run = 0; run < 10; run++)
                out << ";" << results[test][alg][run];
            out << ";" << averages[test][alg] << endl;

            out << endl;
        }
    }

    out << "СВОДНАЯ ТАБЛИЦА (среднее время; мс)" << endl;
    out << ";";
    for (int alg = 0; alg < 3; alg++) {
        out << algNames[alg];
        if (alg < 2) out << ";";
    }
    out << endl;

    for (int test = 0; test < 3; test++) {
        out << testNames[test];
        for (int alg = 0; alg < 3; alg++)
            out << ";" << averages[test][alg];
        out << endl;
    }

    out.close();
}

int main() {
    setlocale(LC_ALL, "Russian");
    srand((unsigned)time(nullptr));

    const int N = 20000000;
    const int Q_SIZE = 15000000;
    const int ITERS = 10;

    int* A = new int[N];
    for (int i = 0; i < N; i++)
        A[i] = 2 * (i + 1);

    int* Q = new int[Q_SIZE];

    double allResults[3][3][10];
    double allAverages[3][3];

    for (int test = 0; test < 3; test++) {
        if (test == 0) {
            cout << "Тест 1 (все найдены)" << endl;
            for (int i = 0; i < Q_SIZE; i++)
                Q[i] = A[i];
        }
        else if (test == 1) {
            cout << "Тест 2 (половина найдена)" << endl;
            for (int i = 0; i < Q_SIZE; i++)
                Q[i] = (i % 2 == 0) ? A[i] : A[i] + 1;
        }
        else {
            cout << "Тест 3 (ни один не найден)" << endl;
            for (int i = 0; i < Q_SIZE; i++)
                Q[i] = 2 * (i + 1) - 1;
        }
        sf(Q, Q_SIZE);

        for (int alg = 0; alg < 3; alg++) {
            double times[ITERS];

            for (int run = 0; run < ITERS; run++) {
                int counter = 0;
                auto start = chrono::high_resolution_clock::now();
                for (int i = 0; i < Q_SIZE; i++) {
                    bool found;
                    if (alg == 0) found = binarySearch1(A, N, Q[i]);
                    else if (alg == 1) found = binarySearch2(A, N, Q[i]);
                    else found = binarySearch3(A, N, Q[i]);
                    if (found) counter++;
                }
                auto end = chrono::high_resolution_clock::now();
                times[run] = chrono::duration<double, milli>(end - start).count();
                allResults[test][alg][run] = times[run];

                if (run == 0)
                    cout << "  Алгоритм " << alg + 1 << " | Прогон " << run + 1
                    << " (отброшен): " << times[run] << " мс, найдено: " << counter << endl;
                else
                    cout << "  Алгоритм " << alg + 1 << " | Прогон " << run + 1
                    << ": " << times[run] << " мс, найдено: " << counter << endl;
            }

            double sum = 0;
            for (int i = 1; i < ITERS; i++)
                sum += times[i];
            double avg = sum / (ITERS - 1);

            int minIdx = 1, maxIdx = 1;
            for (int i = 2; i < ITERS; i++) {
                if (times[i] < times[minIdx]) minIdx = i;
                if (times[i] > times[maxIdx]) maxIdx = i;
            }

            bool hasOutlier = (times[maxIdx] - times[minIdx]) / avg > 0.2;

            if (hasOutlier) {
                sum = 0;
                int cnt = 0;
                for (int i = 1; i < ITERS; i++) {
                    if (i == minIdx || i == maxIdx) continue;
                    sum += times[i];
                    cnt++;
                }
                avg = sum / cnt;
                cout << "  Алгоритм " << alg + 1
                    << " | Среднее (без 1-го, мин, макс): " << avg << " мс" << endl;
            }
            else {
                cout << "  Алгоритм " << alg + 1
                    << " | Среднее (без 1-го): " << avg << " мс" << endl;
            }

            allAverages[test][alg] = avg;
            cout << endl;
        }
    }


    exportToExcel(allResults, allAverages);

    delete[] A;
    delete[] Q;

    return 0;
}
