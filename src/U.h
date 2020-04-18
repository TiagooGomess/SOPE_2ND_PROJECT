#pragma once

#include <stdbool.h>

typedef struct {
    int durationSeconds;
    char * fifoName;
} clientArgs;

void initializeArgumentsStruct(clientArgs * arguments);

bool checkClientArguments(clientArgs * arguments, int argc, char *argv[]);

void testArguments(clientArgs * arguments);