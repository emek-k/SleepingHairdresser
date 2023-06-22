#include <stdio.h>
#include <stdlib.h>
#include "myQueue.h"

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

struct Queue *deleteFirstFromQue(struct Queue *que){
    Que *new = que->next;
    free(que);
    return new;
}

void printQue(struct Queue *que){
    Que *temp = que;
    while(temp != NULL){
        printf("%d | ", temp->id);
        temp = temp->next;
    }
    printf("\n");
}