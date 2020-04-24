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
#include <signal.h>

ClientArgs arguments;
time_t begTime;


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
    //printf("%f\n", difftime(endTime, begTime));
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
    else {}
        //printf("FIFO '%s' sucessfully created\n", privateFifoName);
    
    return true;
}

bool sendRequest(FIFORequest * fArgs, int publicFifoFd) {
    return write(publicFifoFd, fArgs, sizeof(FIFORequest)) > 0;
}

bool receiveMessage(FIFORequest * fRequest, int publicFifoFd) {
    bool readSuccessful;

    readSuccessful = read(publicFifoFd, fRequest, sizeof(FIFORequest)) > 0; // Tries to read answer
    
    if(!readSuccessful) {
        usleep(75 * 1000); // Sleep 75 ms...
        readSuccessful = read(publicFifoFd, fRequest, sizeof(FIFORequest)) > 0; // Tries to read answer again
    }
    return readSuccessful;
}

void * requestServer(void * args) {

    struct stat fileStat;
    stat(arguments.fifoName, &fileStat);
    if(!(fileStat.st_mode & S_IWUSR))
    {
        //printf("FAILD\n...");

        printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), -1, 
            -1, pthread_self(), -1, -1, "FAILD"); // fica -1 ??

        return NULL;
    }
    
    FIFORequest * fRequest = (FIFORequest *) malloc(sizeof(FIFORequest));  // ALready Freed!
    threadArgs * tArgs = (threadArgs *) args;

    fullFillRequest(fRequest, tArgs->seqNum);    

    /* Create Private FIFO */
    char privateFifoName[FIFONAME_MAX_LEN];
    generatePrivateFifoName(fRequest, privateFifoName);

    if(createPrivateFifo(fRequest, privateFifoName)) {
        // printf("Private FIFO Successfully created!\n");
    } 

    // Send Message... (SEE SIGPIPE CASE!) -> FAILD!
    if(sendRequest(fRequest, tArgs->publicFifoFd)) {
        //printf("Client write successful!\n");

        printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
            fRequest->pid, fRequest->tid, fRequest->durationSeconds, fRequest->place, "IWANT");
    }

    int privateFifoFd = open(privateFifoName, O_RDONLY | O_NONBLOCK); // Open is always successful!
    
    // Need to verify time... In order to see if !timeHasPassed. If so, then it closes privateFIFOS. Can make a do-while cicle (with NON-BLOCK -> use fcntl!)

    
    // Receive message...   
    if(receiveMessage(fRequest, privateFifoFd)) {

        printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), fRequest->seqNum, 
            fRequest->pid, fRequest->tid, fRequest->durationSeconds, fRequest->place, "IAMIN");

        if(fRequest->place == -1) {
            printf("C----------------------------------------LOSD--------------------------------------...\n");

            printf("%ld ; %d; %d; %ld; %d; %d; %s\n", time(NULL), -1, 
                -1, pthread_self(), -1, -1, "CLOSD"); // fica -1 ??

        }
    }

    close(privateFifoFd);
    unlink(privateFifoName);
    free(fRequest);
    
    return NULL;
}

int launchRequests(ClientArgs* cliArgs) {
    int fifoFd = openPublicFifo(cliArgs);
    time(&begTime);
    pthread_t threads[MAX_NUM_THREADS]; // Set as an infinite number...
    threadArgs tArgs[MAX_NUM_THREADS];
    int tCounter = 0;
    
    while(!timeHasPassed(cliArgs->durationSeconds, begTime))
    {
        tArgs[tCounter].seqNum = tCounter + 1;
        tArgs[tCounter].publicFifoFd = fifoFd;
        pthread_create(&threads[tCounter], NULL, requestServer, &tArgs[tCounter]);
        tCounter++;
        usleep(25  * 1000); // Sleep for 25 ms between threads...
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
    
    initializeArgumentsStruct(&arguments);

    if(!checkClientArguments(&arguments, argc, argv)) {
        fprintf(stderr, "Error capturing Arguments!\n");
        exit(1);
    }

    generatePublicFifoName(arguments.fifoName);

    // Start request to Server
    int publicFifoFd = launchRequests(&arguments);

    freeMemory(&arguments, publicFifoFd);

    exit(0);
}