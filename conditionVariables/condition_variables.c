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
int elementsInBarberQue;

pthread_mutex_t mutexWaitroom = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCurrentlyCutting = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexClientsLeft = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexQue = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t condBarber = PTHREAD_COND_INITIALIZER;
pthread_cond_t condClient = PTHREAD_COND_INITIALIZER;


void printInfo(){
    pthread_mutex_lock(&mutexWaitroom);
    pthread_mutex_lock(&mutexCurrentlyCutting);
    pthread_mutex_lock(&mutexClientsLeft);
    if(currentlyCutting > 0){
        printf("Resigned: %d     Wait Room: %d/%d     [in:%d]\n", clientsLeft, clientsInWaitingRoom, NUMBER_OF_SEATS_IN_WAITROOM, currentlyCutting);
    }
    else{
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
    barberQue = deleteFirstFromQue(barberQue);
    elementsInBarberQue--;
    pthread_mutex_unlock(&mutexQue);
}

int getFirstElementFromQue(struct Queue *que){
    pthread_mutex_lock(&mutexQue);
    int nextClient = que->id;
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
    //pthread_mutex_lock(&mutexCurrentlyCutting);
    //printf("    Now cutting %d\n", currentlyCutting);
    //pthread_mutex_unlock(&mutexCurrentlyCutting);

    sleep(TIME_OF_CUTTING);

    // pthread_mutex_lock(&mutexCurrentlyCutting);
    // printf("    Done cutting %d\n", currentlyCutting);
    // pthread_mutex_unlock(&mutexCurrentlyCutting);
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

    if(pthread_cond_signal(&condClient) != 0){
        perror("Cond signal condClient error!\n");
    }
}

void* barber(void* args){
    while(true){
        //printf("TEST 1 barber\n");
        while(true){
            //printf("TEST 2 barber\n");
            pthread_mutex_lock(&mutexCurrentlyCutting);
            if(pthread_cond_wait(&condBarber, &mutexCurrentlyCutting) != 0){
            perror("Sem wait semBarber error!\n");
            }
            pthread_mutex_unlock(&mutexCurrentlyCutting);
            //printf("TEST 3 barber\n");
            break;
        }
        //printf("TEST 4 barber\n");
        pthread_mutex_lock(&mutexWaitroom);
        clientsInWaitingRoom--;
        pthread_mutex_unlock(&mutexWaitroom);
        //printf("TEST 5 barber\n");
        //get id of the client from the que
        int clientId = getFirstElementFromQue(barberQue);
        //printf("TEST 5.1 barber\n");
        //set id to currently cutting
        setCurrentlyCutting(clientId);
        //printf("TEST 5.2 barber\n");
        deleteFirstFromBarberQue();
        //printf("TEST 6 barber\n");
        printInfo();
        //printf("TEST 7 barber\n");
        doCutting();
        clientLeft();
        //printf("TEST 8 barber\n");
        //printInfo();
    }

    return NULL;
}


void* customer(void* args){

    long clientId = (long) args;

    printf("Client %ld comes...\n", clientId);
    pthread_mutex_lock(&mutexWaitroom);
    if(clientsInWaitingRoom < NUMBER_OF_SEATS_IN_WAITROOM){
        clientsInWaitingRoom++;
        pthread_mutex_unlock(&mutexWaitroom);
        //printf("TEST 1 client\n");
        addToQueSave(&barberQue, clientId);
        printInfo();
        //printf("TEST 2 client\n");
        pthread_mutex_lock(&mutexCurrentlyCutting);
        while(currentlyCutting != 0){
            //printf("TEST 3 client\n");
            if(pthread_cond_wait(&condClient, &mutexCurrentlyCutting) != 0){
                perror("Cond wait condClient error!\n");
            }
            //printf("TEST 4 client\n");
        }
        pthread_mutex_unlock(&mutexCurrentlyCutting);
        
        //setCurrentlyCutting(clientId);
       // printf("TEST 5 client\n");
        //signal client ready
        if(pthread_cond_signal(&condBarber) != 0){
            perror("Cond signal condBarber error!\n");
        }
        //printf("TEST 6 client\n");

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

void initialazieThreads(){

    srand(time(NULL));

    pthread_t clients[NUMBER_OF_CLIENTS];
    pthread_t barberThread[NUMBER_OF_BARBERS];
    
    for(long i=0; i<NUMBER_OF_BARBERS; i++){
        if(pthread_create(&barberThread[i], NULL, barber, (void*) i)){
                perror("Failed to create barber thread!\n");
                exit(EXIT_FAILURE);
            }
    }

    for(long i=0; i<NUMBER_OF_CLIENTS; i++){
        //randomSleep();
        sleep(1);
        if(pthread_create(&clients[i], NULL, customer, (void*) i + 1)){
            perror("Failed to create customer thread!\n");
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
            perror("Failed to join customer thread!\n");
            exit(EXIT_FAILURE);
        }
    }

    pthread_mutex_destroy(&mutexWaitroom);
    pthread_mutex_destroy(&mutexCurrentlyCutting);
    pthread_mutex_destroy(&mutexClientsLeft);
    pthread_mutex_destroy(&mutexQue);

    pthread_cond_destroy(&condBarber);
    pthread_cond_destroy(&condClient);
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

    initialazieThreads();

    return EXIT_SUCCESS;
}