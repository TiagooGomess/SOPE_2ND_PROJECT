#include "Q1.h"
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

int *buffer;
int slotsAvailable;
pthread_cond_t slots_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t slots_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t slotsInc = PTHREAD_MUTEX_INITIALIZER;

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
    
    buffer = (int *) malloc(serverArguments.numPlaces * sizeof(int)); // Allocate space for bathroom places!
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
    free(buffer);    
}

bool timeHasPassed(int durationSeconds) {
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
        fRequest->place = fRequest->seqNum; // By creating a buffer it results in a critic section (need to use mutexes) - doubtful if should be used in stage 1.
}

void * requestThread(void * args) {
    FIFORequest * fRequest = (FIFORequest *) args;
    printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
        fRequest->pid, fRequest->tid, fRequest->durationSeconds, fRequest->place, "RECVD");
    
    // change time(NULL) ???;
    printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
        fRequest->pid, fRequest->tid, fRequest->durationSeconds, fRequest->place, "ENTER");

    
    usleep(fRequest->durationSeconds * 1000); // Sleep specified number of ms... Time of bathroom.
    printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
        fRequest->pid, fRequest->tid, fRequest->durationSeconds, fRequest->place, "TIMUP");

    char privateFifoName[FIFONAME_MAX_LEN];
    generatePrivateFifoName(fRequest, privateFifoName);
    fullFillMessage(fRequest, false);

    struct stat fileStat;
    int privateFifoFd = -1;
    while(stat(privateFifoName, &fileStat) != ERROR && privateFifoFd == -1) {
        privateFifoFd = open(privateFifoName, O_WRONLY | O_NONBLOCK);
    }    
    
    if(!sendRequest(fRequest, privateFifoFd)) // Server can't answer user... SIGPIPE (GAVUP!) 
    {
        printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
            fRequest->pid, fRequest->tid, fRequest->durationSeconds, -1, "GAVUP");
    }
    close(privateFifoFd);
    return NULL;

}

// Requests out of time... 2LATE 
void * afterClose(void * args) {
    FIFORequest * fRequest = (FIFORequest *) args;
    printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
        fRequest->pid, fRequest->tid, fRequest->durationSeconds, fRequest->place, "RECVD");
    
    char privateFifoName[FIFONAME_MAX_LEN];
    generatePrivateFifoName(fRequest, privateFifoName);
        
    struct stat fileStat;
    int privateFifoFd = -1;
    while(stat(privateFifoName, &fileStat) != ERROR && privateFifoFd == -1) {
        privateFifoFd = open(privateFifoName, O_WRONLY | O_NONBLOCK);
    } 

    fullFillMessage(fRequest, true);
    
    if(sendRequest(fRequest, privateFifoFd))
    {
        printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
            fRequest->pid, fRequest->tid, fRequest->durationSeconds, fRequest->place, "2LATE");
    }
    else 
    {
        printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
            fRequest->pid, fRequest->tid, fRequest->durationSeconds, -1, "GAVUP");
    }

    close(privateFifoFd);
    return NULL;   
}

void receiveRequest(int publicFifoFd) {

    FIFORequest fRequests[MAX_NUM_THREADS];
    pthread_t threads[MAX_NUM_THREADS]; // Set as an infinite number...
    int tCounter = 0;
    bool hasClosed = false, canRead;
    
    do {
        if((canRead = receiveMessage(&fRequests[tCounter], publicFifoFd)) == true) { // We need to make it NON_BLOCK in order for it to close...

            if(!timeHasPassed(serverArguments.numSeconds)) {
                pthread_create(&threads[tCounter], NULL, requestThread, &fRequests[tCounter]);
                usleep(25 * 1000);
            }    
            else {
                pthread_create(&threads[tCounter], NULL, afterClose, &fRequests[tCounter]);

                // Change FIFO Permissions
                if(!hasClosed) {
                    chmod(serverArguments.fifoname, 0400); // Only give read permissions (Server)
                    hasClosed = true;
                }    
            }    

            tCounter++;
        }  
        
    }  while(canRead || !timeHasPassed(serverArguments.numSeconds));

    close(publicFifoFd);
    unlink(serverArguments.fifoname);

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
    
    /* TEMPORARY SOLUTION -> (when nplaces and nthreads is not specified...) 
    The other case that stands for further evaluation is when one of those parameters is not specified (which means infinite)
    If the specific case (when nplaces and nthreads is specified) is well made, then we shouldn't have problems with that!*/
    if(serverArguments.numPlaces == MAX_NUMBER_OF_PLACES && serverArguments.numThreads == MAX_NUM_THREADS)
        receiveRequest(publicFifoFd);
    else 
        receiveSpecRequest(publicFifoFd);    

    freeMemory(publicFifoFd);
    exit(0);
}

void * requestSpecThread(void * args) { // Argument passed is publicFifoFd
    while(true) {

        // First part -> Request can be processed or not
        pthread_mutex_lock(&slots_lock);
        while(!(slotsAvailable > 0)) 
            pthread_cond_wait(&slots_cond, &slots_lock);
        slotsAvailable--;
        pthread_mutex_unlock(&slots_lock);

        FIFORequest fRequest;
        while(!receiveMessage(&fRequest, *(int *) args)); // Protects possible interruption!
            
        printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest.seqNum, fRequest.pid, fRequest.tid, fRequest.durationSeconds, fRequest.place, "RECVD");

        if(!timeHasPassed(serverArguments.numSeconds)) {
            printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest.seqNum, fRequest.pid, fRequest.tid, fRequest.durationSeconds, fRequest.place, "ENTER");
            usleep(fRequest.durationSeconds * 1000); // Sleep specified number of ms... Time of bathroom.
            printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest.seqNum, fRequest.pid, fRequest.tid, fRequest.durationSeconds, fRequest.place, "TIMUP");
        }
        else
            printf("2LATE CASE...\n"); // In development...
        
        char privateFifoName[FIFONAME_MAX_LEN];
        generatePrivateFifoName(&fRequest, privateFifoName);
        fullFillMessage(&fRequest, false);

        struct stat fileStat;
        int privateFifoFd = -1;
        while(stat(privateFifoName, &fileStat) != ERROR && privateFifoFd == -1) {
            privateFifoFd = open(privateFifoName, O_WRONLY | O_NONBLOCK);
        }    
        
        if(!sendRequest(&fRequest, privateFifoFd)) // Server can't answer user... SIGPIPE (GAVUP!) 
        {
            printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest.seqNum, 
                fRequest.pid, fRequest.tid, fRequest.durationSeconds, -1, "GAVUP");
        }

        // SlotsAvailable number is increased
        pthread_mutex_lock(&slotsInc);
        slotsAvailable++;
        pthread_cond_signal(&slots_cond);
        pthread_mutex_unlock(&slotsInc);

        close(privateFifoFd);
        return NULL;
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
    chmod(serverArguments.fifoname, 0400); // Only give read permissions (Server)

    close(publicFifoFd);
    unlink(serverArguments.fifoname);

    for(int tInd = 0; tInd < tCounter; tInd++) { 
        pthread_join(threads[tInd], NULL); // We don't really need to wait for threads, its just because of stdout.
    }
}