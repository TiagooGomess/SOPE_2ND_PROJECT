#include "U2.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h> 
#include <time.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>

ClientArgs arguments;
time_t begTime;

#define MAX_NUM_THREADS 47389 // Can't be a much larger number, otherwise the program blows up!
#define MAX_NUMBER_OF_PLACES 100000

void initializeArgumentsStruct() {
    arguments.durationSeconds = -1;
    arguments.fifoName = (char * ) malloc(FIFONAME_MAX_LEN);
}

bool checkDurationSeconds(int durationSeconds) {
    return (durationSeconds >= 0);
}

bool checkIncrement(int i, int argc) {
    return (i + 1) < argc;
}

bool checkClientArguments(int argc, char *argv[]) {

    if(argc != 4) {
        return false;
    }

    int definedArgs = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0)  {
            if (!checkIncrement(i, argc))
                return false;
            if(sscanf(argv[++i], "%d", &arguments.durationSeconds) <= 0)
                return false;
            definedArgs++;
        }      
        else if (strstr(argv[i], "-") == NULL) {
            strcpy(arguments.fifoName, argv[i]);
            definedArgs++;
        }
    }

    if(!checkDurationSeconds(arguments.durationSeconds))
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

void freeMemory(int publicFifoFd) {
    close(publicFifoFd);
    free(arguments.fifoName);   
}

bool timeHasPassed(int durationSeconds) {
    time_t endTime;
    time(&endTime);
    //printf("%f\n", difftime(endTime, begTime));
    return difftime(endTime, begTime) >= durationSeconds; // in seconds 
}

void fullFillRequest(FIFORequest * fRequest, int seqNum) {
    fRequest->seqNum = seqNum;
    fRequest->pid = getpid();
    fRequest->tid = pthread_self();

    unsigned int rSeed = time(NULL);
    fRequest->durationSeconds = rand_r(&rSeed) % 100 + 75; // rand_r is the reentrant version of rand(). Meaning Thread-Safe!
    fRequest->place = -1;
}

int openPublicFifo() {
    int fifo_fd;

    do {
        
        fifo_fd = open(arguments.fifoName, O_WRONLY);
        
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
    return true;
}

bool sendRequest(FIFORequest * fArgs, int publicFifoFd) {
    return write(publicFifoFd, fArgs, sizeof(FIFORequest)) > 0;
}

bool receiveMessage(FIFORequest * fRequest, int publicFifoFd) {
    bool readSuccessful;

    readSuccessful = read(publicFifoFd, fRequest, sizeof(FIFORequest)) > 0; // Tries to read answer
    if(!readSuccessful) {
        for(int i = 0; i < 10; i++) {
            usleep(150 * 1000); // Sleep 150ms
            readSuccessful = read(publicFifoFd, fRequest, sizeof(FIFORequest)) > 0; // Tries to read answer AGAIN
            if(readSuccessful)
                break;
        }
    }
    return readSuccessful;
}

void * requestServer(void * args) {
    
    FIFORequest * fRequest = (FIFORequest *) malloc(sizeof(FIFORequest));  // ALready Freed!
    threadArgs * tArgs = (threadArgs *) args;

    fullFillRequest(fRequest, tArgs->seqNum);    

    /* Create Private FIFO */
    char privateFifoName[FIFONAME_MAX_LEN];
    generatePrivateFifoName(fRequest, privateFifoName);

    createPrivateFifo(fRequest, privateFifoName);

    // Send Message... (SEE SIGPIPE CASE!) -> FAILD!
    while(!sendRequest(fRequest, tArgs->publicFifoFd));
    printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
        fRequest->pid, fRequest->tid, fRequest->durationSeconds, fRequest->place, "IWANT");
    
    // Need to verify time... In order to see if !timeHasPassed. If so, then it closes privateFIFOS. Can make a do-while cicle (with NON-BLOCK -> use fcntl!)
    int privateFifoFd = open(privateFifoName, O_RDONLY | O_NONBLOCK); // Never fails 

    // Receive message...   
    if(receiveMessage(fRequest, privateFifoFd)) {

        if(fRequest->place != -1) {
            printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
                fRequest->pid, fRequest->tid, fRequest->durationSeconds, fRequest->place, "IAMIN");
        }        
        else {
            printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
                -1, pthread_self(), -1, -1, "CLOSD"); // fica -1 ??
        }
    } 
    else // Client can't receive server's answer.
    {
        printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
            -1, pthread_self(), -1, -1, "FAILD"); // fica -1 ??
    }
    

    close(privateFifoFd);
    unlink(privateFifoName);
    free(fRequest);
    
    return NULL;
}

int launchRequests() {
    int fifoFd = openPublicFifo();
    time(&begTime);
    pthread_t threads[MAX_NUM_THREADS]; // Set as an infinite number...
    threadArgs tArgs[MAX_NUM_THREADS];
    int tCounter = 0;
    
    while(!timeHasPassed(arguments.durationSeconds))
    {
        tArgs[tCounter].seqNum = tCounter + 1;
        tArgs[tCounter].publicFifoFd = fifoFd;
        pthread_create(&threads[tCounter], NULL, requestServer, &tArgs[tCounter]);
        tCounter++;
        usleep(20 * 1000); // Sleep for 20 ms between threads...
    }  

    for(int tInd = 0; tInd < tCounter; tInd++) {
        pthread_join(threads[tInd], NULL);
    }

    return fifoFd;
}

void installSIGHandlers() {
    struct sigaction action;

    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGPIPE,&action, NULL);
     
}

int main(int argc, char* argv[]) {
    installSIGHandlers();
    
    initializeArgumentsStruct();

    if(!checkClientArguments(argc, argv)) {
        fprintf(stderr, "Error capturing Arguments!\n");
        exit(1);
    }

    generatePublicFifoName(arguments.fifoName);

    // Start request to Server
    int publicFifoFd = launchRequests();

    freeMemory(publicFifoFd);

    exit(0);
}