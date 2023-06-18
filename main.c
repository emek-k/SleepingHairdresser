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
#include "myQueue.h"

int NUMBER_OF_BARBERS = 1;   //numer of active hairdresser seats
int NUMBER_OF_SEATS = 5; //capacity of wait room 
int NUMBER_OF_CLIENTS = 10; //number of threads
int TIME_OF_CUTTING = 5; //time it takes to cut a client

int INFO = 0;

int cutting = 0;
int clientsInWaitingRoom = 0;
int currentlyCutting = 0;


pthread_cond_t condCutting, condClientIsReady;
pthread_mutex_t mutexWaitRoom, mutexCurrentlyCutting, mutexCutting;
/*
mutexWaitRoom - clientsInWaitingRoom
mutexCurrentlyCutting - curentlyCutting
mutexCutting - cutting
*/


void printfInfo(){
    if(currentlyCutting > 0){
        printf("Wait Room: %d/%d | In: %d\n", clientsInWaitingRoom, NUMBER_OF_SEATS, currentlyCutting);
        if(INFO == 1){
            printf("Barber que: ");
            printQue(barberQue);
        }
    }
    else{
        printf("Wait Room: %d/%d | In: -\n", clientsInWaitingRoom, NUMBER_OF_SEATS);
        if(INFO == 1){
            printf("Left que: ");
            printQue(leftQue);
        }
            
    }
}

void doCutting(){
    
    pthread_mutex_lock(&mutexCurrentlyCutting);
    pthread_mutex_lock(&mutexCutting);
    cutting = 1;
    pthread_mutex_unlock(&mutexCutting); 
    printf("    Cutting client %d\n", currentlyCutting);
    sleep(TIME_OF_CUTTING);
    pthread_mutex_lock(&mutexCutting);
    cutting = 0;
    pthread_mutex_unlock(&mutexCutting);    
    pthread_mutex_unlock(&mutexCurrentlyCutting);    

    pthread_cond_signal(&condCutting);
}

void setCurrentlyCutting(int clientId){
    pthread_mutex_lock(&mutexCurrentlyCutting);
    currentlyCutting = clientId;
    pthread_mutex_unlock(&mutexCurrentlyCutting);
}

void checkIfBarberIsWorking(int clientId){
    
    pthread_mutex_lock(&mutexCutting);
    if(cutting == 1){
        
        pthread_cond_wait(&condCutting, &mutexCutting);
    }
    pthread_mutex_unlock(&mutexCutting);
    //sometimes when 2 threads signals at the same time, thread who signal last get in
    //to avoid it we add this usleep
    usleep(1000);
    pthread_cond_signal(&condClientIsReady);
    setCurrentlyCutting(clientId);
    
}

void barberAskClientToLeave(){
    pthread_mutex_lock(&mutexCurrentlyCutting);
    printf("    Done cutting client: %d\n", currentlyCutting);
    currentlyCutting = 0;
    pthread_mutex_unlock(&mutexCurrentlyCutting);
    
}

void* barber(void* args){
   while(1){
        printfInfo();
        pthread_mutex_lock(&mutexCutting);
        while(1){
            pthread_cond_wait(&condClientIsReady, &mutexCutting);
            break;
        }
        pthread_mutex_unlock(&mutexCutting);
        pthread_mutex_lock(&mutexWaitRoom);
        clientsInWaitingRoom--;
        pthread_mutex_unlock(&mutexWaitRoom);
        doCutting();
        barberAskClientToLeave();
        printfInfo();
   }
}

void* customer(void* args){


    long clientId = (long) args;
    clientId++;

    
    pthread_mutex_lock(&mutexWaitRoom);
    if(clientsInWaitingRoom < NUMBER_OF_SEATS){
        printf("Client %ld enter waiting room.\n", clientId);
        clientsInWaitingRoom++;

        pthread_mutex_unlock(&mutexWaitRoom);

        barberQue = addToQue(barberQue, clientId);
        printfInfo();

        //checks if barber is working
        checkIfBarberIsWorking(clientId);
    }else{
        printf("Client %ld leaves.\n", clientId);
        leftQue = addToQue(leftQue, clientId);
        printfInfo();
        pthread_mutex_unlock(&mutexWaitRoom);
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

    pthread_cond_init(&condClientIsReady, NULL);
    pthread_cond_init(&condCutting, NULL);
    
    pthread_mutex_init(&mutexCurrentlyCutting, NULL);
    pthread_mutex_init(&mutexCutting, NULL);
    pthread_mutex_init(&mutexWaitRoom, NULL);



    for(long i=0; i<NUMBER_OF_BARBERS; i++){
        if(pthread_create(&barberThread[i], NULL, barber, (void*) i)){
                perror("Failed to create barber thread!\n");
                exit(EXIT_FAILURE);
            }
    }

    for(long i=0; i<NUMBER_OF_CLIENTS; i++){
        randomSleep();
        if(pthread_create(&clients[i], NULL, customer, (void*) i)){
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

    pthread_mutex_destroy(&mutexCurrentlyCutting);
    pthread_mutex_destroy(&mutexCutting);
    pthread_mutex_destroy(&mutexWaitRoom);

    pthread_cond_destroy(&condClientIsReady);
    pthread_cond_destroy(&condCutting);
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