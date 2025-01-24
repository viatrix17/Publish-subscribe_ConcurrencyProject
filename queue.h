#ifndef QUEUE_H    
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

struct List {
    int size;
    void* head;
    void* tail;
};
typedef struct List List;

typedef struct Subscriber Subscriber;

struct Message {
    void* content;
    struct Message* next;
    int readCount; //ma zejsc do zera
    struct Subscriber *firstSub; //wskaznik na pierwszego subskrybenta ktory to odczyta
};
typedef struct Message Message;

typedef struct Subscriber {
    pthread_t *threadID;
    struct Subscriber* next;
    Message *startReading;
    pthread_mutex_t *list_empty; //kazdy subskrybent ma zmienną warunkową i ona jest niezależna od innych subskrybentów, inni subskrybenci mogą się zablokować regardless
    pthread_cond_t *empty;
    //subskrybent nie moze dwa razy tej samej kolejki zasubskrybowac, trzeba to sprawdzać DODAC
}Subscriber;

struct TQueue {
    int maxSize;
    List *msgList;
    List *subList;

    pthread_mutex_t *access_mutex;

    pthread_mutex_t *operation_mutex;
    pthread_cond_t *full;
};
typedef struct TQueue TQueue;




TQueue* createQueue(int *size);

void destroyQueue(TQueue **queue);

void subscribe(TQueue *queue, pthread_t *thread);

void unsubscribe(TQueue *queue, pthread_t *thread);

void addMsg(TQueue *queue, void *msg);

void* getMsg(TQueue *queue, pthread_t *thread);

void getAvailable(TQueue *queue, pthread_t *thread);

void removeMsg(TQueue *queue, void *msg);

void setSize(TQueue *queue, int *newSize);

#endif