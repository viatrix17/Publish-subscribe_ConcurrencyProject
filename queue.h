#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

struct Node {
    void* data;
    Node* next;
};
typedef struct Node Node;

struct List {
    int size;
    Node* head;
    Node* tail;
};
typedef struct List List;
//każda wiadomosc ma liste subskybentow
struct Message {
    void* content;
    Message* next;
    int read; //ile razy została ta wiadomość odczytana
    List* subscribers; //porównywanie czy liczba read zgadza się z subskrybentami, jesli tak, to usuwasz'; List->size == read
};
typedef struct Message Message;

//kazdy subskrybent ma listę wiadomosci do przeczytania
struct subscriber {
    pthread_t *threadID;
    subscriber* next;
    List* messages;
    //subskrybent nie moze dwa razy tej samej kolejki zasubskrybowac, trzeba to sprawdzać
};
typedef struct subscriber subscriber;

struct TQueue {
    int maxSize;
    int size;
    int subCount;
    Message* startMsg;
    Message* lastMsg;
    subscriber* firstSub;
    subscriber* lastSub;
    pthread_mutex_t *operations_mutex;

};
typedef struct TQueue TQueue;