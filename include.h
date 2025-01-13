#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

struct message {
    void* content;
    message* next;
    //coś do przechowywania informacji o tym, czy wiadomość została odczytana przez wszystkich subskrybentów
    //ilosc obecnych subskrybentow dla tej kolejki i licznik odczytanych
};
typedef struct message message;

struct subscriber {
    pthread_t *threadID;
    subscriber* next;
    //subskrybent nie moze dwa razy tej samej kolejki zasubskrybowac, trzeba to sprawdzać
};
typedef struct subscriber subscriber;

struct TQueue {
    int maxSize;
    int size;
    message* startMsg;
    subscriber* firstSub;

};
typedef struct TQueue TQueue;