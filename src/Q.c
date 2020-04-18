#include "Q.h"

#define FIFONAME_MAX_LEN 128

typedef struct {
    int numSeconds;
    int numPlaces;
    int numThreads;
    char* fifoname;
} ServerArguments;

void initializeArgumentsStruct(ServerArguments *serverArguments) {
    serverArguments->numSeconds = 0;
    serverArguments->numPlaces = 0;
    serverArguments->numThreads = 0;
    serverArguments->fifoname = (char*) malloc(FIFONAME_MAX_LEN*sizeof(char));
    strcpy(serverArguments->fifoname, "");
}

bool checkServerArguments(ServerArguments * arguments, int argc, char* argv[]) {

    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0)  {
            sscanf(argv[++i], "%d", &arguments->numSeconds);
            continue;
        }
        if (strcmp(argv[i], "-l") == 0)  {
            sscanf(argv[++i], "%d", &arguments->numPlaces);
            continue;
        }
        if (strcmp(argv[i], "-t") == 0)  {
            sscanf(argv[++i], "%d", &arguments->numThreads);
            continue;
        }     
        else {
            strcpy(arguments->fifoname, argv[i]);
        }
            
    }
    return true;
}

int main(int argc, char* argv[]) {
    
    if (argc != 8) {
        fprintf(stderr, "Number of arguments wrong\n");
        exit(1);
    }
        

    ServerArguments serverArguments;
    initializeArgumentsStruct(&serverArguments);

    checkServerArguments(&serverArguments, argc, argv);

    printf("Num Seconds: %d\n", serverArguments.numSeconds);
    printf("Num Places: %d\n", serverArguments.numPlaces);
    printf("Num Threads: %d\n", serverArguments.numThreads);
    printf("Fifoname: %s\n", serverArguments.fifoname);
    
    
    exit(0);
}