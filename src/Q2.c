#include "Q2.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/file.h> 
#include <limits.h>
#include <signal.h>

#define MAX_NUM_THREADS 250 // Can't be a much larger number, otherwise the program blows up!
#define MAX_NUMBER_OF_PLACES 500

int buffer[MAX_NUMBER_OF_PLACES];
int slotsAvailable;
pthread_cond_t slots_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t slots_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;

time_t begTime;
ServerArguments serverArguments;


void initializeArgumentsStruct() {
    serverArguments.numSeconds = -1;
    serverArguments.numPlaces = MAX_NUMBER_OF_PLACES;
    serverArguments.numThreads = MAX_NUM_THREADS;
    serverArguments.fifoname = (char*) malloc(FIFONAME_MAX_LEN*sizeof(char));
}

bool checkIncrement(int i, int argc) {
    return (i + 1) < argc;
}

bool checkServerArguments(int argc, char *argv[]) {
    
    if(argc < 4) {
        return false;
    }
    int definedArgs = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            if (!checkIncrement(i, argc))
                return false;
            if (sscanf(argv[++i], "%d", &serverArguments.numThreads) <= 0) 
                return false;
            
            continue;
        }
        if (strcmp(argv[i], "-l") == 0) {
            if (!checkIncrement(i, argc))
                return false;
            if (sscanf(argv[++i], "%d", &serverArguments.numPlaces) <= 0) 
                return false;
            
            continue;
        }
        if (strcmp(argv[i], "-t") == 0)  {
            if (!checkIncrement(i, argc))
                return false;
            if (sscanf(argv[++i], "%d", &serverArguments.numSeconds) <= 0) 
                return false;
            definedArgs++;
            continue;
        }     
        else if (strstr(argv[i], "-") == NULL) {
            definedArgs++;
            if(definedArgs > 2)
                return false;
            strcpy(serverArguments.fifoname, argv[i]);        
        } 
        else
            return false;
    }

    if(definedArgs != 2)
        return false;

    if(serverArguments.numPlaces > MAX_NUMBER_OF_PLACES)
        serverArguments.numPlaces = MAX_NUMBER_OF_PLACES;
    if(serverArguments.numThreads > MAX_NUM_THREADS)
        serverArguments.numThreads = MAX_NUM_THREADS;

    for(int index = 0; index < serverArguments.numPlaces; index++) // Initialize buffer elements with -1 (unoccupied)
        buffer[index] = -1;

    slotsAvailable = serverArguments.numPlaces;

    return true;
}

void generatePublicFifoName(char * fifoName) {
    char tempFifoName[FIFONAME_MAX_LEN];
    strcpy(tempFifoName, fifoName);
    sprintf(fifoName, "/tmp/%s", tempFifoName);
}

bool createPublicFifo() {

    generatePublicFifoName(serverArguments.fifoname);

    if (mkfifo(serverArguments.fifoname, 0660) < 0) {
        if (errno == EEXIST)
            fprintf(stderr, "FIFO '%s' already exists\n", serverArguments.fifoname);
        else
            fprintf(stderr, "Can't create FIFO\n");
        return false;
    }
    else 
        printf("FIFO '%s' sucessfully created\n", serverArguments.fifoname);
    
    return true;
}

void freeMemory(int publicFifoFd) {
    free(serverArguments.fifoname);
}

bool timeHasPassed(int durationSeconds) {
    time_t endTime;
    time(&endTime);
    // printf("%f\n", difftime(endTime, begTime));
    return difftime(endTime, begTime) >= durationSeconds; // in seconds 
}

void generatePrivateFifoName(FIFORequest * fArgs, char * toReceive) {
    sprintf(toReceive, "/tmp/%d.%ld", fArgs->pid, fArgs->tid);
}

bool sendRequest(FIFORequest * fRequest, int privateFifoFd) {
    return write(privateFifoFd, fRequest, sizeof(FIFORequest)) > 0;
}

void installSIGHandlers() {
    struct sigaction action;

    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGPIPE,&action, NULL);
     
}

void receiveSpecRequest(int publicFifoFd);

int main(int argc, char* argv[]) {
    time(&begTime);
    installSIGHandlers();

    initializeArgumentsStruct();

    if (!checkServerArguments(argc, argv)) {
        fprintf(stderr, "Error capturing Arguments!\n");
        exit(2);
    }

    /* Create Public FIFO */
    if (!createPublicFifo()) {
        fprintf(stderr, "Error while creating FIFO\n");
        exit(3);
    }
    
    int publicFifoFd = open(serverArguments.fifoname, O_RDONLY | O_NONBLOCK);
    
    printf("nPlaces: %d\n", serverArguments.numPlaces);
    printf("nThreads: %d\n", serverArguments.numThreads);
    receiveSpecRequest(publicFifoFd);    
    
    freeMemory(publicFifoFd);
    exit(0);
}

int receiveSpecMessage(FIFORequest * fArgs, int publicFifoFd) {
    return read(publicFifoFd, fArgs, sizeof(FIFORequest));
}

int getAvailablePlace() {
    int index = 0;
    while(true) {
        if(buffer[index] == -1) {
            // printf("SELECTED INDEX: %d\n", index);
            buffer[index] = 0; // Occupied place...
            return index;
        }
        index = (index + 1) % serverArguments.numPlaces;
    }
}

void fullFillSpecMessage(FIFORequest * fRequest, bool afterClose) {
    fRequest->pid = getpid();
    fRequest->tid = pthread_self();
    
    if(afterClose) {
        fRequest->durationSeconds = -1;
        fRequest->place = -1;
    }    
    else {
        fRequest->durationSeconds = fRequest->durationSeconds;
        pthread_mutex_lock(&buffer_lock);
        fRequest->place = getAvailablePlace(); 
        pthread_mutex_unlock(&buffer_lock);
    }
}

void releaseSlot() {
    pthread_mutex_lock(&slots_lock);
    slotsAvailable++;
    pthread_cond_signal(&slots_cond);
    pthread_mutex_unlock(&slots_lock);
}

void releasePlace(FIFORequest* fRequest) {
    pthread_mutex_lock(&buffer_lock);
    buffer[fRequest->place] = -1;
    pthread_mutex_unlock(&buffer_lock);
}

void * requestSpecThread(void * args) { // Argument passed is publicFifoFd
    FIFORequest fRequest, toSend;
    int canRead;

    while(true) {

        // First part -> Request can be processed or not - Queue
        pthread_mutex_lock(&slots_lock);
        while(!(slotsAvailable > 0)) 
            pthread_cond_wait(&slots_cond, &slots_lock);
        slotsAvailable--;
        pthread_mutex_unlock(&slots_lock);
        
        while(true) {
            canRead = receiveSpecMessage(&fRequest, *(int *) args);
            if(canRead == -1) { // When reading is interrupted, repeat...
                continue;
            }    
            else if(canRead == 0) { // Means that there are no more requests to be read (0 bytes)
                if(timeHasPassed(serverArguments.numSeconds)) { // End-Condition
                    releaseSlot();
                
                    return NULL;
                }

            }
            else // Successful reading!
                break;
        } 

        toSend = fRequest;
        printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest.seqNum, fRequest.pid, fRequest.tid, fRequest.durationSeconds, fRequest.place, "RECVD");

        char privateFifoName[FIFONAME_MAX_LEN];
        generatePrivateFifoName(&fRequest, privateFifoName);

        int privateFifoFd = open(privateFifoName, O_WRONLY | O_NONBLOCK);

        bool endedTime = timeHasPassed(serverArguments.numSeconds);
        if(!endedTime)
            fullFillSpecMessage(&toSend, false);
         else 
            fullFillSpecMessage(&toSend, true);

        if(!sendRequest(&toSend, privateFifoFd)) // Server can't answer user... SIGPIPE (GAVUP!) 
        {
            printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest.seqNum, 
                fRequest.pid, fRequest.tid, fRequest.durationSeconds, -1, "GAVUP");

            // Place gets available again!
            releasePlace(&toSend);
            
            // SlotsAvailable number is increased
            releaseSlot();

            close(privateFifoFd);

            continue;
        }       // (R*)

        if(!endedTime) {
            printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest.seqNum, fRequest.pid, fRequest.tid, fRequest.durationSeconds, fRequest.place, "ENTER");
            usleep(fRequest.durationSeconds * 1000); // Sleep specified number of ms... Time of bathroom.
            printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest.seqNum, fRequest.pid, fRequest.tid, fRequest.durationSeconds, fRequest.place, "TIMUP");
        }
        else {
            printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest.seqNum, 
                fRequest.pid, fRequest.tid, fRequest.durationSeconds, fRequest.place, "2LATE");
        }

        // Place gets available again!
        releasePlace(&toSend);
        
        // SlotsAvailable number is increased
        releaseSlot();
        
        close(privateFifoFd);
   
    }
}

void receiveSpecRequest(int publicFifoFd) {

    pthread_t threads[serverArguments.numThreads]; // Set as an infinite number...
    int tCounter = 0;
    
    for(int nThreads = 0; nThreads < serverArguments.numThreads; nThreads++) {
        pthread_create(&threads[tCounter], NULL, requestSpecThread, &publicFifoFd);
        tCounter++;
    }

    sleep(serverArguments.numSeconds);

    for(int tInd = 0; tInd < tCounter; tInd++) { 
        pthread_join(threads[tInd], NULL); // We don't really need to wait for threads, its just because of stdout.
    }

    close(publicFifoFd);
    unlink(serverArguments.fifoname);
}


 