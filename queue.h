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

typedef struct List {
    int size;
    void* head;
    void* tail;
}List;

typedef struct Subscriber Subscriber;

typedef struct Message {
    void* content;
    struct Message* next;
    int readCount; 
    struct Subscriber *firstSub; 
}Message;

typedef struct Subscriber {
    pthread_t *threadID;
    struct Subscriber* next;
    Message *startReading;
}Subscriber;

typedef struct TQueue {
    int maxSize;
    List *msgList;
    List *subList;
    //mutex for read/write operations synchronization
    pthread_mutex_t *access_mutex;
    //mutex and conditional variable for blocking behaviour of addMsg() and getMsg()
    pthread_mutex_t *operation_mutex;
    pthread_cond_t *block_operation;
}TQueue;




TQueue* createQueue(int *size);

void destroyQueue(TQueue **queue);

void subscribe(TQueue *queue, pthread_t *thread);

void unsubscribe(TQueue *queue, pthread_t *thread);

void addMsg(TQueue *queue, void *msg);

void* getMsg(TQueue *queue, pthread_t *thread);

int getAvailable(TQueue *queue, pthread_t *thread);

void removeMsg(TQueue *queue, void *msg);

void setSize(TQueue *queue, int *newSize);

#endif