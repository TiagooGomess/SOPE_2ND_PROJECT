#include "Q.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h> 
#include <limits.h>

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
        else if (strstr(argv[i], "-") == NULL) {
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

void generatePublicFifoName(char * fifoName) {
    char tempFifoName[FIFONAME_MAX_LEN];
    strcpy(tempFifoName, fifoName);
    sprintf(fifoName, "/tmp/%s", tempFifoName);
}

bool createPublicFifo(ServerArguments *arguments) {

    generatePublicFifoName(arguments->fifoname);

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

void freeMemory(ServerArguments * arguments, int publicFifoFd) {
    // unlink(arguments->fifoname);
    close(publicFifoFd);
    free(arguments->fifoname);
    
}

bool timeHasPassed(int durationSeconds, time_t begTime) {
    time_t endTime;
    time(&endTime);
    // printf("%f\n", difftime(endTime, begTime));
    return difftime(endTime, begTime) >= durationSeconds; // in seconds 
}

bool receiveMessage(FIFORequest * fArgs, int publicFifoFd) {
    return read(publicFifoFd, fArgs, sizeof(FIFORequest)) > 0;
}

void generatePrivateFifoName(FIFORequest * fArgs, char * toReceive) {
    sprintf(toReceive, "/tmp/%d.%ld", fArgs->pid, fArgs->tid);
}

bool sendRequest(FIFORequest * fRequest, int privateFifoFd) {
    return write(privateFifoFd, fRequest, sizeof(fRequest)) > 0;
}

void fullFillMessage(FIFORequest * fRequest) {
    fRequest->pid = getpid();
    fRequest->tid = pthread_self();
    fRequest->durationSeconds = -1;
    fRequest->place = INT_MAX; // By creating a buffer it results in a critic section (need to use mutexes) - doubtful if should be used in stage 1.
}

void * requestThread(void * args) {
    FIFORequest * fRequest = (FIFORequest *) args;

    printf("SeqNum: %d\n", fRequest->seqNum);
    printf("PID: %d\n", fRequest->pid);
    printf("TID: %ld\n", fRequest->tid);
    printf("Duration: %d\n", fRequest->durationSeconds);
    printf("Place: %d\n", fRequest->place);

    usleep(fRequest->durationSeconds * 1000); // Sleep specified number of ms... Time of bathroom.

    char privateFifoName[FIFONAME_MAX_LEN];
    generatePrivateFifoName(fRequest, privateFifoName);
    int privateFifoFd = open(privateFifoName, O_WRONLY);

    fullFillMessage(fRequest);
    
    sendRequest(fRequest, privateFifoFd);

    return NULL;

}

void receiveRequest(int publicFifoFd, ServerArguments* serverArguments, time_t begTime) {
    FIFORequest fRequests[MAX_NUM_THREADS];
    pthread_t threads[MAX_NUM_THREADS]; // Set as an infinite number...
    int tCounter = 0;
    
    while(!timeHasPassed(serverArguments->numSeconds, begTime)) {
        
        if(receiveMessage(&fRequests[tCounter], publicFifoFd)) {
            printf("Server message received!\n");
            pthread_create(&threads[tCounter], NULL, requestThread, &fRequests[tCounter]);
            tCounter++;
        }    
    }  

    // Still need to verify pending requests!
    for(int tInd = 0; tInd < tCounter; tInd++) { 
        pthread_join(threads[tInd], NULL); // We don't really need to wait for threads, it just because of stdout.
    }
}

int main(int argc, char* argv[]) {
    
    ServerArguments serverArguments;
    time_t begTime;
    time(&begTime);

    initializeArgumentsStruct(&serverArguments);

    if (!checkServerArguments(&serverArguments, argc, argv)) {
        fprintf(stderr, "Error capturing Arguments!\n");
        exit(2);
    }

    /* Create Public FIFO */
    if (!createPublicFifo(&serverArguments)) {
        fprintf(stderr, "Error while creating FIFO\n");
        exit(3);
    }
    
    int publicFifoFd = open(serverArguments.fifoname, O_RDONLY);

    receiveRequest(publicFifoFd, &serverArguments, begTime);

    sleep(5);

    freeMemory(&serverArguments, publicFifoFd);
    exit(0);
}