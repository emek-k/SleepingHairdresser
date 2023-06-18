#ifndef EXAMPLE_H
#define EXAMPLE_H

typedef struct Queue{
    int id;
    pthread_cond_t turn;
    struct Queue *next; 
}Que;

Que *barberQue;
Que *leftQue;

struct Queue *addToQue(struct Queue **que, int clientId);
struct Queue *getClientFromQue();
void printQue(struct Queue *que);

#endif