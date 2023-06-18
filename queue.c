#include <stdio.h>
#include <stdlib.h>
#include "myQueue.h"


Que *barberQue = NULL;
Que *leftQue = NULL;

struct Queue *addToQue(struct Queue *que, int clientId){

    Que *new = malloc(sizeof(Que));
    new->id = clientId;
    new->next = NULL;

    if(que == NULL){
        que = new;
    }else{
        Que *temp = que;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = new;
    }
    return que;
}


void printQue(struct Queue *que){
    Que *temp = que;
    while(temp != NULL){
        printf("ID: %d | ", temp->id);
        temp = temp->next;
    }
    printf("\n");
}