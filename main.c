//TODO
//add leftQue
//reorganize code
//add one more header with all threads operations

/*
Problem śpiącego fryzjera
Autorzy - Emanuel Korycki, Patryk Rybak
*/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "myThredHeader.h"

int NUMBER_OF_BARBERS = 1;   //numer of active hairdresser seats
int NUMBER_OF_SEATS = 5; //capacity of wait room 
int NUMBER_OF_CLIENTS = 10; //number of threads
int TIME_OF_CUTTING = 5; //time it takes to cut a client

int INFO = 1;

int cutting = 0;
int clientsInWaitingRoom = 0;
int currentlyCutting = 0;


pthread_cond_t condCutting, condClientIsReady;

int nextClient = 0;

pthread_mutex_t mutexBarberQue;
pthread_mutex_t mutexBarber;
pthread_mutex_t mutexWaitRoom, mutexCurrentlyCutting, mutexCutting;
/*
mutexWaitRoom - clientsInWaitingRoom
mutexCurrentlyCutting - curentlyCutting
mutexCutting - cutting
*/


void printfInfo(){
    if(currentlyCutting > 0){
        printf("W8: %d/%d | In: %d\n", clientsInWaitingRoom, NUMBER_OF_SEATS, currentlyCutting);
    }
    else{
        printf("W8: %d/%d | In: -\n", clientsInWaitingRoom, NUMBER_OF_SEATS);
    }
}

void doCutting(){
    
    pthread_mutex_lock(&mutexCutting);
    pthread_mutex_lock(&mutexCurrentlyCutting);
    cutting = 1;
    printf("    Cutting client %d\n", currentlyCutting);
    sleep(TIME_OF_CUTTING);
    cutting = 0;
    pthread_mutex_unlock(&mutexCutting);    
    pthread_mutex_unlock(&mutexCurrentlyCutting);    

    pthread_cond_signal(&condCutting);
}


void checkIfBarberIsWorking(){
    pthread_mutex_lock(&mutexCutting);
    while(cutting == 1){
        pthread_cond_wait(&condCutting, &mutexCutting);
    }
    pthread_mutex_unlock(&mutexCutting);
}

void barberAskClientToLeave(){
    pthread_mutex_lock(&mutexCurrentlyCutting);
    currentlyCutting = 0;
    pthread_mutex_unlock(&mutexCurrentlyCutting);
}

void getNextClient(){
    //current client, first client on the que
    Que *client = getClientFromQue();
    pthread_mutex_lock(&mutexCurrentlyCutting);
    currentlyCutting = client->id;
    pthread_mutex_unlock(&mutexCurrentlyCutting);
    pthread_cond_signal((pthread_cond_t*)&client->turn);
}

void* barber(void* args){
   while(1){
    pthread_mutex_lock(&mutexWaitRoom);

    //client has to wake up barber every time
    while(clientsInWaitingRoom == 0){
        pthread_cond_wait(&condClientIsReady, &mutexWaitRoom);
    }
    clientsInWaitingRoom--;
    
    pthread_mutex_unlock(&mutexWaitRoom);

    getNextClient();
    doCutting();

    pthread_mutex_lock(&mutexCurrentlyCutting);
    printf("    Done cutting client %d\n", currentlyCutting);
    pthread_mutex_unlock(&mutexCurrentlyCutting);

    barberAskClientToLeave();
   }
}

void* customer(void* args){
    long clientId = (long) args;
    clientId++;

    pthread_mutex_lock(&mutexWaitRoom);

    if(clientsInWaitingRoom < NUMBER_OF_SEATS){
        printf("Client %ld enter waiting room.\n", clientId);
        clientsInWaitingRoom++;

        pthread_mutex_lock(&mutexBarberQue);
        Que *client = addToQue(&barberQue, clientId);
        pthread_mutex_unlock(&mutexBarberQue);

        pthread_mutex_unlock(&mutexWaitRoom);

        printfInfo();


        // //checks if barber is working
        // checkIfBarberIsWorking();

        //wakes barber to cut him
        pthread_cond_signal(&condClientIsReady);

        //wait for our turn
        pthread_mutex_lock(&mutexBarber);
        while(currentlyCutting != clientId){
            pthread_cond_wait((pthread_cond_t*)&client->turn, &mutexBarber);
        }
        pthread_mutex_unlock(&mutexBarber);

        checkIfBarberIsWorking();

        
    }else{
        printf("Client %ld leaves.\n", clientId);
        printfInfo();
        pthread_mutex_unlock(&mutexWaitRoom);
    }
}


void initialazieThreads(){
    pthread_t clients[NUMBER_OF_CLIENTS];
    pthread_t barberThread[NUMBER_OF_BARBERS];

    pthread_cond_init(&condClientIsReady, NULL);
    pthread_cond_init(&condCutting, NULL);
    


    pthread_mutex_init(&mutexCurrentlyCutting, NULL);
    pthread_mutex_init(&mutexCutting, NULL);
    pthread_mutex_init(&mutexWaitRoom, NULL);

    pthread_mutex_init(&mutexBarberQue, NULL);
    pthread_mutex_init(&mutexBarber, NULL);



    for(long i=0; i<NUMBER_OF_BARBERS; i++){
        if(pthread_create(&barberThread[i], NULL, barber, (void*) i)){
                perror("Failed to create barber thread!\n");
                exit(EXIT_FAILURE);
            }
    }

    for(long i=0; i<NUMBER_OF_CLIENTS; i++){
        sleep(2);
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
    pthread_mutex_destroy(&mutexBarber);

    pthread_mutex_destroy(&mutexBarberQue);

    pthread_cond_destroy(&condClientIsReady);
    pthread_cond_destroy(&condCutting);
}

int main(int argc, char *argv[]){

    // if(argc == 2 && strcmp(argv[1], "-info") == 0){
    //     //pokazujemy wszystkie informacjie
    // }else{
    //     createThreads();
    // }

    initialazieThreads();

    
    return 0;
}