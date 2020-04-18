#include "Q.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



void initializeArgumentsStruct(ServerArguments *serverArguments) {
    serverArguments->numSeconds = 0;
    serverArguments->numPlaces = 0;
    serverArguments->numThreads = 0;
    serverArguments->fifoname = (char*) malloc(FIFONAME_MAX_LEN*sizeof(char));
}

bool checkServerArguments(ServerArguments *arguments, int argc, char *argv[]) {

    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            if (sscanf(argv[++i], "%d", &arguments->numSeconds) <= 0) {
                return false;
            }
            continue;
        }
        if (strcmp(argv[i], "-l") == 0) {
            if (sscanf(argv[++i], "%d", &arguments->numPlaces) <= 0) {
                return false;
            }
            continue;
        }
        if (strcmp(argv[i], "-t") == 0)  {
            if (sscanf(argv[++i], "%d", &arguments->numThreads) <= 0) {
                return false;
            }
            continue;
        }     
        else {
            strcpy(arguments->fifoname, argv[i]);
        }   
    }
    return true;
}

int main(int argc, char* argv[]) {
    
    ServerArguments serverArguments;
    initializeArgumentsStruct(&serverArguments);

    if (!checkServerArguments(&serverArguments, argc, argv)) {
        fprintf(stderr, "Erro nos argumentos\n");
        exit(2);
    }

    printf("Num Seconds: %d\n", serverArguments.numSeconds);
    printf("Num Places: %d\n", serverArguments.numPlaces);
    printf("Num Threads: %d\n", serverArguments.numThreads);
    printf("Fifoname: %s\n", serverArguments.fifoname);
    
    
    exit(0);
}