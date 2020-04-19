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
#include <errno.h>

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

void generatePublicFifoName(char * fifoName) {
    char tempFifoName[FIFONAME_MAX_LEN];
    strcpy(tempFifoName, fifoName);
    sprintf(fifoName, "/tmp/%s", tempFifoName);
}

void freeMemory(ClientArgs * arguments, int publicFifoFd) {
    close(publicFifoFd);
    free(arguments->fifoName);   
}

bool timeHasPassed(int durationSeconds, time_t begTime) {
    time_t endTime;
    time(&endTime);
    printf("%f\n", difftime(endTime, begTime));
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

void generatePrivateFifoName(FIFORequest * fArgs, char * toReceive) {
    sprintf(toReceive, "/tmp/%d.%ld", fArgs->pid, fArgs->tid);
}

bool createPrivateFifo(FIFORequest * fArgs, char * privateFifoName) {

    if (mkfifo(privateFifoName, 0660) < 0) {
        if (errno == EEXIST)
            fprintf(stderr, "FIFO '%s' already exists\n", privateFifoName);
        else
            fprintf(stderr, "Can't create FIFO\n");
        return false;
    }
    else 
        printf("FIFO '%s' sucessfully created\n", privateFifoName);
    
    return true;
}

bool sendRequest(FIFORequest * fArgs, int publicFifoFd) {
    return write(publicFifoFd, fArgs, sizeof(FIFORequest)) > 0;
}

bool receiveMessage(FIFORequest * fRequest, int publicFifoFd) {
    return read(publicFifoFd, fRequest, sizeof(FIFORequest)) > 0;
}

void * requestServer(void * args) {
    FIFORequest * fRequest = (FIFORequest *) malloc(sizeof(FIFORequest));  // ALready Freed!
    threadArgs * tArgs = (threadArgs *) args;

    fullFillRequest(fRequest, tArgs->seqNum);    

    /* Create Private FIFO */
    char privateFifoName[FIFONAME_MAX_LEN];
    generatePrivateFifoName(fRequest, privateFifoName);

    if(createPrivateFifo(fRequest, privateFifoName)) {
        printf("Private FIFO Successfully created!\n");
    }   
    
    // Send Message...
    if(sendRequest(fRequest, tArgs->publicFifoFd)) {
        printf("Client write successful!\n");
    }

    int privateFifoFd = open(privateFifoName, O_RDONLY);
    // Receive message...

    if(receiveMessage(fRequest, privateFifoFd)) {
        printf("SeqNum: %d\n", fRequest->seqNum);
        printf("PID: %d\n", fRequest->pid);
        printf("TID: %ld\n", fRequest->tid);
        printf("Duration: %d\n", fRequest->durationSeconds);
        printf("Place: %d\n", fRequest->place);
    }

    close(privateFifoFd);
    unlink(privateFifoName);
    free(fRequest);

    return NULL;
}

int launchRequests(ClientArgs* cliArgs, time_t begTime) {
    int fifoFd = openPublicFifo(cliArgs);
    pthread_t threads[MAX_NUM_THREADS]; // Set as an infinite number...
    threadArgs tArgs[MAX_NUM_THREADS];
    int tCounter = 0;
    
    while(!timeHasPassed(cliArgs->durationSeconds, begTime))
    {
        tArgs[tCounter].seqNum = tCounter + 1;
        tArgs[tCounter].publicFifoFd = fifoFd;
        pthread_create(&threads[tCounter], NULL, requestServer, &tArgs[tCounter]);
        tCounter++;
        usleep(500 * 1000); // Sleep for 100 ms between threads...
    }  

    for(int tInd = 0; tInd < tCounter; tInd++) {
        pthread_join(threads[tInd], NULL);
    }

    return fifoFd;
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

    generatePublicFifoName(arguments.fifoName);

    // Start request to Server
    int publicFifoFd = launchRequests(&arguments, begTime);

    freeMemory(&arguments, publicFifoFd);

    exit(0);
}