/****************************************************************************
 *
 *  mutex_test.cpp
 *      ($C:\_repo\np_alloc\np_alloc_test)
 *
 *  by icedac 
 *     (2018/7/26)
 *
 ***/
#include "stdafx.h"
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>

const int tastCount = 160000;
int numThreads;
const int MAX_THREADS = 16;

double g_shmem = 8;
std::mutex g_mutex;
CRITICAL_SECTION g_critSec;

void sharedFunc(int i, double &data)
{
    for (int j = 0; j < 100; j++)
    {
        if (j % 2 == 0)
            data = sqrt(data);
        else
            data *= data;
    }
}

void threadFuncCritSec() {
    double lMem = 8;
    int iterations = tastCount / numThreads;
    for (int i = 0; i < iterations; ++i) {
        for (int j = 0; j < 100; j++)
            sharedFunc(j, lMem);
        EnterCriticalSection(&g_critSec);
        sharedFunc(i, g_shmem);
        LeaveCriticalSection(&g_critSec);
    }
    printf("results: %f\n", lMem);
}

void threadFuncMutex() {
    double lMem = 8;
    int iterations = tastCount / numThreads;
    for (int i = 0; i < iterations; ++i) {
        for (int j = 0; j < 100; j++)
            sharedFunc(j, lMem);
        g_mutex.lock();
        sharedFunc(i, g_shmem);
        g_mutex.unlock();
    }
    printf("results: %f\n", lMem);
}

void testRound()
{
    std::vector<std::thread> threads;

    auto startMutex = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numThreads; ++i)
        threads.push_back(std::thread(threadFuncMutex));
    for (std::thread& thd : threads)
        thd.join();
    auto endMutex = std::chrono::high_resolution_clock::now();

    std::cout << "std::mutex:       ";
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(endMutex - startMutex).count();
    std::cout << "ms \n\r";

    threads.clear();
    auto startCritSec = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numThreads; ++i)
        threads.push_back(std::thread(threadFuncCritSec));
    for (std::thread& thd : threads)
        thd.join();
    auto endCritSec = std::chrono::high_resolution_clock::now();

    std::cout << "CRITICAL_SECTION: ";
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(endCritSec - startCritSec).count();
    std::cout << "ms \n\r";
}

void mutex_test() 
{
    InitializeCriticalSection(&g_critSec);

    std::cout << "Tasks: " << tastCount << "\n\r";

    for (numThreads = 1; numThreads <= MAX_THREADS; numThreads = numThreads * 2) {
        if (numThreads == 16)
            numThreads = 12;
        Sleep(100);
        std::cout << "Thread count: " << numThreads << "\n\r";
        testRound();
    }

    DeleteCriticalSection(&g_critSec);
}
