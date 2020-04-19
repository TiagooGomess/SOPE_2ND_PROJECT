#include "U.h"
#include "macros.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h> 
#include <time.h>
#include <limits.h>

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
        else if (strstr(argv[i], "-") == NULL) {
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

void freeMemory(ClientArgs * arguments) {
    free(arguments->fifoName);   
}

bool timeHasPassed(int durationSeconds, time_t begTime) {
    time_t endTime;
    time(&endTime);
    // printf("%f\n", difftime(endTime, begTime));
    return difftime(endTime, begTime) >= durationSeconds; // in seconds 
}

void fullFillRequest(FIFORequest * fRequest, int seqNum) {
    fRequest->seqNum = seqNum;
    fRequest->pid = getpid();
    fRequest->tid = pthread_self();

    unsigned int rSeed = time(NULL);
    fRequest->durationSeconds = rand_r(&rSeed) % 100; // rand_r is the reentrant version of rand(). Meaning Thread-Safe!
    fRequest->place = -1;
}

int openPublicFifo(ClientArgs * cliArgs) {
    int fifo_fd;

    do {
        fifo_fd = open(cliArgs->fifoName, O_WRONLY);
        
        if(fifo_fd == -1)
            sleep(1);

    } while(fifo_fd == -1);

    return fifo_fd;
}

bool sendRequest(FIFORequest * fArgs, int publicFifoFd) {
    return write(publicFifoFd, fArgs, sizeof(FIFORequest)) > 0;
}

void * requestServer(void * args) {
    void * result = malloc(sizeof(FIFORequest));
    FIFORequest * fRequest = (FIFORequest *) result;
    threadArgs * tArgs = (threadArgs *) args;

    fullFillRequest(fRequest, tArgs->seqNum);    
    if(sendRequest(fRequest, tArgs->publicFifoFd)) {
        printf("Client write successful!\n");
    }

    return NULL;
}

void launchRequests(ClientArgs* cliArgs, time_t begTime) {
    int fifoFd = openPublicFifo(cliArgs);
    pthread_t threads[MAX_NUM_THREADS]; // Set as an infinite number...
    threadArgs tArgs[MAX_NUM_THREADS];
    int tCounter = 0;
    
    while(!timeHasPassed(cliArgs->durationSeconds, begTime))
    {
        tArgs[tCounter].seqNum = tCounter + 1;
        tArgs[tCounter].publicFifoFd = fifoFd;
        printf("ANOTHER\n");
        pthread_create(&threads[tCounter], NULL, requestServer, &tArgs[tCounter]);
        tCounter++;
        usleep(100 * 1000); // Sleep for 100 ms between threads...
    }  

    for(int tInd = 0; tInd < tCounter; tInd++) {
        pthread_join(threads[tInd], NULL); // We don't really need to wait for threads, it just because of stdout.
    }
}

int main(int argc, char* argv[]) {
    
    ClientArgs arguments;
    time_t begTime;
    time(&begTime);

    initializeArgumentsStruct(&arguments);

    if(!checkClientArguments(&arguments, argc, argv)) {
        fprintf(stderr, "Error capturing Arguments!\n");
        exit(1);
    }

    generateFifoName(arguments.fifoName);

    // Start request to Server
    launchRequests(&arguments, begTime);

    freeMemory(&arguments);

    exit(0);
}