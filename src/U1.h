#pragma once

#include <stdbool.h>
#include "macros.h"

typedef struct {
    int durationSeconds;
    char * fifoName;
} ClientArgs;

void initializeArgumentsStruct();

bool checkClientArguments(int argc, char *argv[]);

bool checkIncrement(int i, int argc);

bool checkDurationSeconds(int durationSeconds);

void generatePublicFifoName(char * fifoName);

void freeMemory(int publicFifoFd);

bool timeHasPassed(int durationSeconds);

void fullFillRequest(FIFORequest * fRequest, int seqNum);

int openPublicFifo();

void generatePrivateFifoName(FIFORequest * fArgs, char * toReceive);

bool createPrivateFifo(FIFORequest * fArgs, char * privateFifoName);

bool sendRequest(FIFORequest * fArgs, int publicFifoFd);

bool receiveMessage(FIFORequest * fRequest, int publicFifoFd);

void * requestServer(void * args);

int launchRequests();

void installSIGHandlers();