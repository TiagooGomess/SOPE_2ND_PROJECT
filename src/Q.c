#include "Q.h"
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
    return write(privateFifoFd, fRequest, sizeof(FIFORequest)) > 0;
}

void fullFillMessage(FIFORequest * fRequest, bool afterClose) {
    fRequest->pid = getpid();
    fRequest->tid = pthread_self();
    fRequest->durationSeconds = -1;
    
    if(afterClose)
        fRequest->place = -1;
    else
        fRequest->place = INT_MAX; // By creating a buffer it results in a critic section (need to use mutexes) - doubtful if should be used in stage 1.
}

void * requestThread(void * args) {
    printf("...\n");
    FIFORequest * fRequest = (FIFORequest *) args;

    // change time(NULL) ???;
    printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
        fRequest->pid, fRequest->tid, fRequest->durationSeconds, fRequest->place, "ENTER");

    
    usleep(fRequest->durationSeconds * 1000); // Sleep specified number of ms... Time of bathroom.
    printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
        fRequest->pid, fRequest->tid, fRequest->durationSeconds, fRequest->place, "TIMUP");

    char privateFifoName[FIFONAME_MAX_LEN];
    generatePrivateFifoName(fRequest, privateFifoName);
    fullFillMessage(fRequest, false);

    int privateFifoFd = open(privateFifoName, O_WRONLY);
    
    if(!sendRequest(fRequest, privateFifoFd)) // Server can't answer user... SIGPIPE (GAVUP!) 
    {
        printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), -1, 
            -1, pthread_self(), -1, -1, "GAVUP"); // fica -1 ??
    }
    printf("2\n");
    close(privateFifoFd);
    return NULL;

}

// Requests out of time... 2LATE (could be optimized...)
void * afterClose(void * args) {
    printf("AFTER\n");
    FIFORequest * fRequest = (FIFORequest *) args;

    /*
    printf("SeqNum: %d\n", fRequest->seqNum);
    printf("PID: %d\n", fRequest->pid);
    printf("TID: %ld\n", fRequest->tid);
    printf("Duration: %d\n", fRequest->durationSeconds);
    printf("Place: %d\n", fRequest->place);*/


    char privateFifoName[FIFONAME_MAX_LEN];
    generatePrivateFifoName(fRequest, privateFifoName);
        
    int privateFifoFd = open(privateFifoName, O_WRONLY);

    fullFillMessage(fRequest, true);
    
    if(sendRequest(fRequest, privateFifoFd))
    {
        printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
            fRequest->pid, fRequest->tid, fRequest->durationSeconds, fRequest->place, "2LATE");
    }

    printf("3\n");

    close(privateFifoFd);
    return NULL;   
}

void receiveRequest(int publicFifoFd, ServerArguments* serverArguments) {
    time_t begTime;
    time(&begTime);

    FIFORequest fRequests[MAX_NUM_THREADS];
    pthread_t threads[MAX_NUM_THREADS]; // Set as an infinite number...
    int tCounter = 0;
    bool hasClosed = false, canRead;
    
    do {
        if((canRead = receiveMessage(&fRequests[tCounter], publicFifoFd)) == true) { // We need to make it NON_BLOCK in order for it to close...
            //printf("Server message received!\n");

            printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), -1, 
                -1, (long) -1, -1, -1, "RECVD"); // fRequest ainda não é conhecida...... fica -1 ??


            if(!timeHasPassed(serverArguments->numSeconds, begTime))
                pthread_create(&threads[tCounter], NULL, requestThread, &fRequests[tCounter]);
            else {
                pthread_create(&threads[tCounter], NULL, afterClose, &fRequests[tCounter]);

                // Change FIFO Permissions
                if(!hasClosed) {
                    chmod(serverArguments->fifoname, 0400); // Only give read permissions (Server)
                    hasClosed = true;
                }    
            }    

            tCounter++;
        }  
    }  while(canRead);

    close(publicFifoFd);
    unlink(serverArguments->fifoname);

    for(int tInd = 0; tInd < tCounter; tInd++) { 
        pthread_join(threads[tInd], NULL); // We don't really need to wait for threads, it just because of stdout.
    }
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
    ServerArguments serverArguments;

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

    
    receiveRequest(publicFifoFd, &serverArguments);

    freeMemory(&serverArguments, publicFifoFd);
    pthread_exit(NULL);
}