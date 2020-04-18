#include "Q.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



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

int main(int argc, char* argv[]) {
    
    ServerArguments serverArguments;
    initializeArgumentsStruct(&serverArguments);

    if (!checkServerArguments(&serverArguments, argc, argv)) {
        fprintf(stderr, "Error capturing Arguments!\n");
        exit(2);
    }

    printf("Num Seconds: %d\n", serverArguments.numSeconds);
    printf("Num Places: %d\n", serverArguments.numPlaces);
    printf("Num Threads: %d\n", serverArguments.numThreads);
    printf("Fifoname: %s\n", serverArguments.fifoname);
    
    
    exit(0);
}