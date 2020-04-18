#pragma once

#include "macros.h"
#include <stdbool.h>

typedef struct {
    int durationSeconds;
    char * fifoName;
} clientArgs;

void initializeArguments(clientArgs * arguments);

bool checkArguments(clientArgs * arguments, int argc, char *argv[]);

void testArguments(clientArgs * arguments);