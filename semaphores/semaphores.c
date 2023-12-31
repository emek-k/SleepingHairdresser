/*
Problem śpiącego fryzjera
Autorzy - Emanuel Korycki, Patryk Rybak

To run a program write:
make
./run 10 3 -info
10 - number of clients
3 - time of cutting
-info - (not necessary) flag to show barberQue and leftQue
*/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/shm.h>
#include "myQueue.h"

int NUMBER_OF_BARBERS = 1;   //numer of active hairdresser seats
int NUMBER_OF_SEATS_IN_WAITROOM = 5; //capacity of wait room 
int NUMBER_OF_CLIENTS = 10; //number of threads
int TIME_OF_CUTTING = 5; //time it takes to cut a client

int INFO = 0;

int clientsInWaitingRoom = 0;
int currentlyCutting = 0;
int clientsLeft = 0;
int elementsInBarberQue = 0;
int nextClient = 0;

pthread_mutex_t mutexWaitroom = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCurrentlyCutting = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexClientsLeft = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexQue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexNextClient = PTHREAD_MUTEX_INITIALIZER;

sem_t *semClient; //signal client is ready
sem_t *semBarber; //wakes barber up


void printInfo(){
    pthread_mutex_lock(&mutexWaitroom);
    pthread_mutex_lock(&mutexCurrentlyCutting);
    pthread_mutex_lock(&mutexClientsLeft);
    if(currentlyCutting > 0){
        printf("Resigned: %d     Wait Room: %d/%d     [in:%d]\n", clientsLeft, clientsInWaitingRoom, NUMBER_OF_SEATS_IN_WAITROOM, currentlyCutting);
    }
    else if(currentlyCutting == 0){
        printf("Resigned: %d     Wait Room: %d/%d     [in:-]\n", clientsLeft, clientsInWaitingRoom, NUMBER_OF_SEATS_IN_WAITROOM);
    }
    pthread_mutex_unlock(&mutexClientsLeft);
    pthread_mutex_unlock(&mutexCurrentlyCutting);
    pthread_mutex_unlock(&mutexWaitroom);

    if(INFO == 1){
        pthread_mutex_lock(&mutexQue);
        printf("        BarberQue: ");
        printQue(barberQue);
        printf("        LeftQue: ");
        printQue(leftQue);
        pthread_mutex_unlock(&mutexQue);
    }
}

void deleteFirstFromBarberQue(){
    pthread_mutex_lock(&mutexQue);
    if(elementsInBarberQue > 0){
        barberQue = deleteFirstFromQue(barberQue);
        elementsInBarberQue--;
    }
    pthread_mutex_unlock(&mutexQue);
}

int getFirstElementFromQue(struct Queue *que){
    pthread_mutex_lock(&mutexQue);
    int nextClient = 0;
    if(elementsInBarberQue > 0){
        nextClient = que->id;
    }
    pthread_mutex_unlock(&mutexQue);
    return nextClient;
}

void addToQueSave(struct Queue **que, long clientId){
    pthread_mutex_lock(&mutexQue);
    *que = addToQue(*que, clientId);

    if(*que == barberQue)
        elementsInBarberQue++;

    pthread_mutex_unlock(&mutexQue);
}

void doCutting(){
    sleep(TIME_OF_CUTTING);
}

void takeClientFromWaitRoom(){
    pthread_mutex_lock(&mutexWaitroom);
    clientsInWaitingRoom--;
    pthread_mutex_unlock(&mutexWaitroom);
}

void setCurrentlyCutting(long clientId){
    pthread_mutex_lock(&mutexCurrentlyCutting);
    currentlyCutting = clientId;
    pthread_mutex_unlock(&mutexCurrentlyCutting);
}

void clientLeft(){
    pthread_mutex_lock(&mutexCurrentlyCutting);
    currentlyCutting = 0;
    pthread_mutex_unlock(&mutexCurrentlyCutting);
}

void* barber(void* args){
    while(true){
        pthread_mutex_lock(&mutexWaitroom);
        while(clientsInWaitingRoom == 0){
            pthread_mutex_unlock(&mutexWaitroom);
            if(sem_wait(semBarber) != 0){
                perror("Sem wait semBarber error!\n");
            }
        }
        pthread_mutex_unlock(&mutexWaitroom);
        //take next client id
        int clientId = getFirstElementFromQue(barberQue);
        //signal to the nextClient that barber is ready
        pthread_mutex_lock(&mutexNextClient);
        nextClient = clientId;
        pthread_mutex_unlock(&mutexNextClient);
        if(sem_post(semClient) != 0){
            perror("Cond signal condClient error!\n");
        }
        takeClientFromWaitRoom();
        doCutting();
        clientLeft();
        printInfo();
    }
    return NULL;
}


void* client(void* args){

    long clientId = (long) args;
    printf("Client %ld comes...\n", clientId);
    pthread_mutex_lock(&mutexWaitroom);
    if(clientsInWaitingRoom < NUMBER_OF_SEATS_IN_WAITROOM){
        clientsInWaitingRoom++;
        pthread_mutex_unlock(&mutexWaitroom);
        addToQueSave(&barberQue, clientId);
        printInfo();

        //signal client ready
        if(sem_post(semBarber) != 0){
            perror("Sem post semBarber error!\n");
        }

        //wait for client turn
        pthread_mutex_lock(&mutexNextClient);
        while(nextClient != clientId){
            pthread_mutex_unlock(&mutexNextClient);
            if(sem_wait(semClient) != 0){
                perror("Cond wait condClient error!\n");
            }
            pthread_mutex_lock(&mutexNextClient);
        }
        pthread_mutex_unlock(&mutexNextClient);
    
        //set id to currently cutting
        setCurrentlyCutting(clientId);
        
        //client goes for cutting
        //printf("TEST 1 CUST\n");
        deleteFirstFromBarberQue();
        printInfo();
    }else{
        pthread_mutex_unlock(&mutexWaitroom);
        pthread_mutex_lock(&mutexClientsLeft);
        clientsLeft++;
        pthread_mutex_unlock(&mutexClientsLeft);
        addToQueSave(&leftQue, clientId);
        printInfo();
    }
    return NULL;
}

void randomSleep(){
    int randomTime = rand() % 5 + 1;
    sleep(randomTime);
}

void initializeThreads(){

    srand(time(NULL));

    pthread_t clients[NUMBER_OF_CLIENTS];
    pthread_t barberThread[NUMBER_OF_BARBERS];

    sem_unlink("/semClient");
    sem_unlink("/semBarber");

    semBarber = sem_open("/semBarber", O_CREAT|O_EXCL, S_IRWXU, 0);
    semClient = sem_open("/semClient", O_CREAT|O_EXCL, S_IRWXU, 0);
    
    for(long i=0; i<NUMBER_OF_BARBERS; i++){
        if(pthread_create(&barberThread[i], NULL, barber, (void*) i)){
                perror("Failed to create barber thread!\n");
                exit(EXIT_FAILURE);
            }
    }

    for(long i=0; i<NUMBER_OF_CLIENTS; i++){
        randomSleep();
        //sleep(1);
        if(pthread_create(&clients[i], NULL, client, (void*) i + 1)){
            perror("Failed to create client thread!\n");
            exit(EXIT_FAILURE);
        }
    }
    
    for(long i=0; i<NUMBER_OF_BARBERS; i++){
        if(pthread_join(barberThread[i], NULL)){
            perror("Failed to join barber thread!\n");
            exit(EXIT_FAILURE);
        }
    }

    for(long i=0; i<NUMBER_OF_CLIENTS; i++){
        if(pthread_join(clients[i], NULL)){
            perror("Failed to join client thread!\n");
            exit(EXIT_FAILURE);
        }
    }

    if(pthread_mutex_destroy(&mutexWaitroom) != 0){
        perror("Failed to destroy mutex mutexWaitroom!\n");
    }
    if(pthread_mutex_destroy(&mutexCurrentlyCutting) != 0){
         perror("Failed to destroy mutex mutexCurrentlyCutting!\n");
    }
    if(pthread_mutex_destroy(&mutexClientsLeft) != 0){
         perror("Failed to destroy mutex mutexClientsLeft!\n");
    }
    if(pthread_mutex_destroy(&mutexQue) != 0){
         perror("Failed to destroy mutex mutexQue!\n");
    }
    if(pthread_mutex_destroy(&mutexNextClient) != 0){
         perror("Failed to destroy mutex mutexQue!\n");
    }
    if(sem_close(semClient) != 0){
         perror("Failed to destroy semaphore semClient!\n");
    }
    if(sem_close(semBarber) != 0){
         perror("Failed to destroy semaphore semBarber!\n");
    }
}

int main(int argc, char *argv[]){

    if(argc < 2){
        printf("Expected number of clients (threads) and time of cutting!\n");
        return EXIT_FAILURE;
    }

    if(argc < 3){
        printf("Expected time of cutting!\n");
        return EXIT_FAILURE;
    }


    if(isdigit(*argv[1])){
        NUMBER_OF_CLIENTS = atoi(argv[1]);
    }else{
        printf("Wrong value. Expected a number of clients (threads)!\n");
        return EXIT_FAILURE;
    }

    if(isdigit(*argv[2])){
        TIME_OF_CUTTING = atoi(argv[2]);
    }else{
        printf("Wrong value. Expected time of cutting!\n");
        return EXIT_FAILURE;
    }

    if(argc > 3 && (strcmp(argv[3], "-info") == 0 || strcmp(argv[3], "-INFO") == 0)){
        INFO = 1;
    }

    printf("STATUS | CLIENTS %d | CUT TIME %d | INFO %d\n", NUMBER_OF_CLIENTS, TIME_OF_CUTTING, INFO);

    initializeThreads();

    return EXIT_SUCCESS;
}