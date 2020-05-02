#pragma once
#include <stdbool.h>
#include "macros.h"

typedef struct {
    int numSeconds;
    int numPlaces;
    int numThreads;
    char* fifoname;
} ServerArguments;

void initializeArgumentsStruct(ServerArguments *serverArguments);

bool checkServerArguments(ServerArguments*  arguments, int argc, char* argv[]);

bool checkIncrement(int i, int argc);

void generatePublicFifoName(char * fifoName);

bool createPublicFifo(ServerArguments *arguments);

void freeMemory(ServerArguments * arguments, int publicFifoFd);

bool timeHasPassed(int durationSeconds, time_t begTime);

bool receiveMessage(FIFORequest * fArgs, int publicFifoFd);

void generatePrivateFifoName(FIFORequest * fArgs, char * toReceive);

bool sendRequest(FIFORequest * fRequest, int privateFifoFd);

void fullFillMessage(FIFORequest * fRequest, bool afterClose);

void * requestThread(void * args);

void * afterClose(void * args);

void receiveRequest(int publicFifoFd, ServerArguments* serverArguments);

void installSIGHandlers();