#pragma once
#include <pthread.h>
#include <unistd.h>

#define FIFONAME_MAX_LEN 128
#define MAX_NUM_THREADS 47389 // Can't be a much larger number, otherwise the program blows up!
#define MAX_NUMBER_OF_PLACES 100000
#define ERROR -1

typedef struct {
    int seqNum;
    pid_t pid;
    pthread_t tid;
    int durationSeconds;
    int place; // Doesn't really matter on the 1st Stage.

} FIFORequest;

typedef struct {
    int seqNum;
    int publicFifoFd;
} threadArgs;
