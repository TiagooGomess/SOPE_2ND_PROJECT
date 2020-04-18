#include "U.h"
#include "macros.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h> 
#include <unistd.h>

void initializeArgumentsStruct(ClientArgs * arguments) {
    arguments->durationSeconds = -1;
    arguments->fifoName = (char * ) malloc(FIFONAME_MAX_LEN);
}

bool checkDurationSeconds(int durationSeconds) {
    return (durationSeconds >= 0);
}

bool checkIncrement(int i, int argc) {
    return (i + 1) < argc;
}

bool checkClientArguments(ClientArgs * arguments, int argc, char *argv[]) {

    if(argc != 4) {
        return false;
    }

    int definedArgs = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0)  {
            if (!checkIncrement(i, argc))
                return false;
            if(sscanf(argv[++i], "%d", &arguments->durationSeconds) <= 0)
                return false;
            definedArgs++;
        }      
        else if (strstr(argv[i], "-") == NULL && strstr(argv[i], ".") == NULL) {
            strcpy(arguments->fifoName, argv[i]);
            definedArgs++;
        }
    }

    if(!checkDurationSeconds(arguments->durationSeconds))
        return false;

    if(definedArgs != 2)
        return false;

    return true;
}

void generateFifoName(char * fifoName) {
    char tempFifoName[FIFONAME_MAX_LEN];
    strcpy(tempFifoName, fifoName);
    sprintf(fifoName, "/tmp/%s", tempFifoName);
}

int main(int argc, char* argv[]) {
    
    ClientArgs arguments;
    initializeArgumentsStruct(&arguments);

    if(!checkClientArguments(&arguments, argc, argv)) {
        fprintf(stderr, "Error capturing Arguments!\n");
        exit(1);
    }
    
    int fifo_fd;
    generateFifoName(arguments.fifoName);

    printf("Opening client...\n");
    printf("%s\n", arguments.fifoName);
    
    do {
        fifo_fd = open(arguments.fifoName, O_WRONLY);
        
        if(fifo_fd == -1)
            sleep(1);

    } while(fifo_fd == -1);
        
    printf("%d\n", fifo_fd);

    printf("Working hard...\n");

    exit(0);
}