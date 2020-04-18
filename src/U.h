#pragma once

#include <stdbool.h>

typedef struct {
    int durationSeconds;
    char * fifoName;
} ClientArgs;

void initializeArgumentsStruct(ClientArgs * arguments);

bool checkClientArguments(ClientArgs * arguments, int argc, char *argv[]);

void testArguments(ClientArgs * arguments);