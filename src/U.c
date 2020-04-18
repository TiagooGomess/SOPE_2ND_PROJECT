#include "U.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initializeArguments(clientArgs * arguments) {
    arguments->durationSeconds = -1;
    arguments->fifoName = (char * ) malloc(MAX_FIFO_NAME);
}

bool checkDurationSeconds(int durationSeconds) {
    return (durationSeconds >= 0);
}

bool checkArguments(clientArgs * arguments, int argc, char *argv[]) {

    if(argc != 4) {
        return false;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0)  {
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
    initializeArguments(&arguments);
    if(!checkArguments(&arguments, argc, argv)) {
        fprintf(stderr, "Error capturing Arguments!\n");
        exit(1);
    }
    
    // Test
    testArgumentsReceived(&arguments);
    
    exit(0);
}