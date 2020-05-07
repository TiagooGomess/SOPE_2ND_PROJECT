#pragma once
#include <stdbool.h>
#include "macros.h"

typedef struct {
    int numSeconds;
    int numPlaces;
    int numThreads;
    char* fifoname;
} ServerArguments;

void initializeArgumentsStruct();

bool checkServerArguments(int argc, char* argv[]);

bool checkIncrement(int i, int argc);

void generatePublicFifoName(char * fifoName);

bool createPublicFifo();

void freeMemory(int publicFifoFd);

bool timeHasPassed(int durationSeconds);

bool receiveMessage(FIFORequest * fArgs, int publicFifoFd);

void generatePrivateFifoName(FIFORequest * fArgs, char * toReceive);

bool sendRequest(FIFORequest * fRequest, int privateFifoFd);

void fullFillMessage(FIFORequest * fRequest, bool afterClose);

void * requestThread(void * args);

void * afterClose(void * args);

void receiveRequest(int publicFifoFd);

void installSIGHandlers();