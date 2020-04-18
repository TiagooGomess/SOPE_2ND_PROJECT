#include "Q.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h> 



void initializeArgumentsStruct(ServerArguments *serverArguments) {
    serverArguments->numSeconds = -1;
    serverArguments->numPlaces = 0;
    serverArguments->numThreads = 0;
    serverArguments->fifoname = (char*) malloc(FIFONAME_MAX_LEN*sizeof(char));
}

bool checkIncrement(int i, int argc) {
    return (i + 1) < argc;
}

bool checkServerArguments(ServerArguments *arguments, int argc, char *argv[]) {
    
    if(argc < 4) {
        return false;
    }
    int definedArgs = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            if (!checkIncrement(i, argc))
                return false;
            if (sscanf(argv[++i], "%d", &arguments->numThreads) <= 0) 
                return false;
            
            continue;
        }
        if (strcmp(argv[i], "-l") == 0) {
            if (!checkIncrement(i, argc))
                return false;
            if (sscanf(argv[++i], "%d", &arguments->numPlaces) <= 0) 
                return false;
            
            continue;
        }
        if (strcmp(argv[i], "-t") == 0)  {
            if (!checkIncrement(i, argc))
                return false;
            if (sscanf(argv[++i], "%d", &arguments->numSeconds) <= 0) 
                return false;
            definedArgs++;
            continue;
        }     
        else if (strstr(argv[i], "-") == NULL && strstr(argv[i], ".") == NULL) {
            definedArgs++;
            if(definedArgs > 2)
                return false;
            strcpy(arguments->fifoname, argv[i]);        
        } 
        else
            return false;
    }

    if(definedArgs != 2)
        return false;
    
    return true;
}

void generateFifoName(char * fifoName) {
    char tempFifoName[FIFONAME_MAX_LEN];
    strcpy(tempFifoName, fifoName);
    sprintf(fifoName, "/tmp/%s", tempFifoName);
}

bool createPublicFifo(ServerArguments *arguments) {

    generateFifoName(arguments->fifoname);

    if (mkfifo(arguments->fifoname, 0660) < 0) {
        if (errno == EEXIST)
            fprintf(stderr, "FIFO '%s' already exists\n", arguments->fifoname);
        else
            fprintf(stderr, "Can't create FIFO\n");
        return false;
    }
    else 
        printf("FIFO '%s' sucessfully created\n", arguments->fifoname);
    
    return true;
}

void freeMemory(ServerArguments * arguments) {
    // unlink(arguments->fifoname);
    free(arguments->fifoname);
    
}

int main(int argc, char* argv[]) {
    
    ServerArguments serverArguments;
    initializeArgumentsStruct(&serverArguments);

    if (!checkServerArguments(&serverArguments, argc, argv)) {
        fprintf(stderr, "Error capturing Arguments!\n");
        exit(2);
    }

    if (!createPublicFifo(&serverArguments)) {
        fprintf(stderr, "Error while creating fifo\n");
        exit(3);
    }
    
    int fifo_fd;
    fifo_fd = open(serverArguments.fifoname, O_RDONLY);
    printf("%d\n", fifo_fd);

    freeMemory(&serverArguments);
    exit(0);
}