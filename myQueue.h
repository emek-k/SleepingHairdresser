#ifndef EXAMPLE_H
#define EXAMPLE_H

typedef struct Queue{
    int id;
    struct Queue *next; 
}Que;

Que *barberQue;
Que *leftQue;

struct Queue *addToQue(struct Queue *que, int clientId);
struct Queue *deleteFirstFromQue(struct Queue *que);
void printQue(struct Queue *que);

#endif