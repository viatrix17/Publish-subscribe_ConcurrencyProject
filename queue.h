#ifndef LCL_QUEUE_H    
#define LCL_QUEUE_H

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
typedef struct Message Message;

struct Message {
    void* content;
    struct Message* next;
    int readCount; 
    struct Subscriber *firstSub; 
};

struct Subscriber {
    pthread_t threadID;
    struct Subscriber* next;
    Message *startReading;
};

struct TQueue {
    int maxSize;
    List *msgList;
    List *subList;
    //mutex for read/write operations synchronization and conditional variable to implement blocking behaviour of addMsg() and gettMsg()
    pthread_mutex_t *access_mutex;
    pthread_cond_t *block_operation;
};
typedef struct TQueue TQueue;



TQueue* createQueue(int size);

void destroyQueue(TQueue *queue);

void subscribe(TQueue *queue, pthread_t thread);

void unsubscribe(TQueue *queue, pthread_t thread);

void addMsg(TQueue *queue, void *msg);

void* getMsg(TQueue *queue, pthread_t thread);

int getAvailable(TQueue *queue, pthread_t thread);

void removeMsg(TQueue *queue, void *msg);

void setSize(TQueue *queue, int newSize);

#endif