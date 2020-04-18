#pragma once
#include <stdbool.h>

typedef struct {
    int numSeconds;
    int numPlaces;
    int numThreads;
    char* fifoname;
} ServerArguments;

void initializeArgumentsStruct(ServerArguments *serverArguments);

bool checkServerArguments(ServerArguments*  arguments, int argc, char* argv[]);