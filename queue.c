#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "myThredHeader.h"


Que *barberQue = NULL;
Que *leftQue = NULL;

struct Queue *addToQue(struct Queue **que, int clientId){

    Que *new = malloc(sizeof(Que));
    new->id = clientId;
    pthread_cond_init((pthread_cond_t*)&new->turn, NULL);
    new->next = NULL;

    if(*que == NULL){
        *que = new;
    }else{
        Que *temp = *que;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = new;
    }
    return new;
}

struct Queue *getClientFromQue(){
    Que *tmp = barberQue;
	barberQue = barberQue->next;
	return tmp;
}

void printQue(struct Queue *que){
    Que *temp = que;
    while(temp != NULL){
        printf("ID: %d | ", temp->id);
        temp = temp->next;
    }
    printf("\n");
}