#include "U.h"
#include "macros.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initializeArgumentsStruct(clientArgs * arguments) {
    arguments->durationSeconds = -1;
    arguments->fifoName = (char * ) malloc(FIFONAME_MAX_LEN);
}

bool checkDurationSeconds(int durationSeconds) {
    return (durationSeconds >= 0);
}

bool checkIncrement(int i, int argc) {
    return (i + 1) < argc;
}

bool checkClientArguments(clientArgs * arguments, int argc, char *argv[]) {

    if(argc != 4) {
        return false;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0)  {
            if (!checkIncrement(i, argc))
                return false;
            if(sscanf(argv[++i], "%d", &arguments->durationSeconds) <= 0)
                return false;
        }      
        else 
            strcpy(arguments->fifoName, argv[i]);
    }

    if(!checkDurationSeconds(arguments->durationSeconds))
        return false;

    return true;
}

void testArgumentsReceived(clientArgs * arguments) {
    printf("Duration: %d\n", arguments->durationSeconds);
    printf("Fifo Name: %s\n", arguments->fifoName);
}

int main(int argc, char* argv[]) {
    
    clientArgs arguments;
    initializeArgumentsStruct(&arguments);
    if(!checkClientArguments(&arguments, argc, argv)) {
        fprintf(stderr, "Error capturing Arguments!\n");
        exit(1);
    }
    
    // Test
    testArgumentsReceived(&arguments);
    
    exit(0);
}