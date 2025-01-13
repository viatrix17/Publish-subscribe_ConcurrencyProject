#include "include.h"

extern pthread_mutex_t operations_mutex;

TQueue* createQueue(int *size) { 
    pthread_mutex_lock(&operations_mutex);
    TQueue* queue = (TQueue*)malloc(sizeof(TQueue));
    if (queue == NULL) { 
        printf("Memory allocation failed.\n");
        return NULL;
    }
    queue->maxSize = size;
    queue->startMsg = NULL;
    queue->firstSub = NULL;
    queue->size = 0;
    return queue;
    pthread_mutex_unlock(&operations_mutex);
}

void destroyQueue(TQueue **queue) {
    pthread_mutex_lock(&operations_mutex);
    if (queue == NULL)  {
        return; //jesli juz nic nie ma, to nie ma czego zwolnic
    }
    //nic nie moze przeszkodzic
    message *currMsg = (*queue)->startMsg, *tempMsg;
    subscriber* currSub = (*queue)->firstSub, *tempSub;
    while (currMsg != NULL && currSub != NULL) {
        
        tempMsg = currMsg;
        tempSub = currSub;
        currMsg = currMsg->next;
        currSub = currSub->next;
        free(tempMsg); //zwolnienie pamieci dla elementu
        free(tempSub);
    }
    free(*queue);
    *queue = NULL;
    pthread_mutex_unlock(&operations_mutex);
}

void subscribe(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&operations_mutex);

    subscriber* newSubscriber = (subscriber*)malloc(sizeof(subscriber));
    if (newSubscriber = NULL) {
        printf("Memory allocation failed!\n");
        pthread_mutex_unlock(&operations_mutex);
        return;
    }
    newSubscriber->threadID = thread;
    newSubscriber->next = NULL;
    
    if(queue->firstSub == NULL) {
        queue->firstSub = newSubscriber;
    }
    else {
        subscriber* curr = queue->firstSub;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = newSubscriber; 
    }
    pthread_mutex_unlock(&operations_mutex);
}

void unsubscribe(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&operations_mutex);
    if (queue == NULL || queue->firstSub == NULL) {
        pthread_mutex_unlock(&operations_mutex);
        printf("exiting critical section remove\n");
        return 1;
    }
    subscriber* curr = queue->firstSub;
    subscriber* prev;
    while (curr->next != NULL) {
        if (curr->threadID == thread) {
            prev->next = curr->next;
            printf("Element removed!\n");
            queue->size--;
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&operations_mutex);
    printf("exiting critical section remove\n");
    printf("Element not found!\n");
    return 0;
}

void addMsg(TQueue *queue, void *msg) { //dodac semafor
    printf("entering critical section put\n");
    pthread_mutex_lock(&operations_mutex);
    //sem_wait(size_sem); //SEMAFOR DODAC po opusczeniu zamka, bo inaczej dwa mogą opuscic dodajc wiadomosc
    printf("in put\n");
    message* newMsg = (message*)malloc(sizeof(message)); //owning the memory it's pointing to
    if (newMsg == NULL) {
        printf("Memory allocation failed!\n");
        pthread_mutex_unlock(&operations_mutex);
        return;
    }
    newMsg->content = msg;
    newMsg->next = NULL;

    if (queue->startMsg == NULL) {
        queue->startMsg = newMsg;
    }
    else {
        message* curr = queue->startMsg;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = newMsg;
    }
    queue->size++; 
    pthread_mutex_unlock(&operations_mutex);
    printf("exiting critical section put\n");
}

// void* getMsg(TQueue *queue, pthread_t *thread) { //dodać obsługiwanie wątków
//     printf("enters critical section get\n");
//     pthread_mutex_lock(&operations_mutex);
//     printf("in get\n");
//     if (list == NULL || list->start == NULL) {
//         printf("the list is empty. Can't get the first item.\n");
//         pthread_mutex_unlock(&operations_mutex);
//         printf("exiting critical section get\n");
//         return NULL;
//     }
//     element* firstItem = list->start;
//     list->start = list->start->next;
//     //what about deleteing 
//     list->size--;
//     printf("shit\n");
//TUTAJ GDZIEŚ JEST PODNOSZONY SEMAFOR, ŻEBY SIE DAŁO WRZUCIĆ NASTĘPNĄ WIADOMOŚĆ
//     pthread_mutex_unlock(&operations_mutex);
//     printf("exiting critical section get\n");
//     return firstItem;
// }

// getAvailable(TQueue *queue, pthread_t *thread) {

// }

void removeMsg(TQueue *queue, void *msg) { //dodac semafor
    printf("entering critical section remove\n");
    pthread_mutex_lock(&operations_mutex);
    printf("in remove\n");
    if (queue == NULL || queue->startMsg == NULL) {
        pthread_mutex_unlock(&operations_mutex);
        printf("exiting critical section remove\n");
        return 1;
    }
    message* curr = queue->startMsg;
    message* prev;
    while (curr->next != NULL) {
        if (curr->content == msg) {
            prev->next = curr->next;
            printf("Element removed!\n");
            queue->size--;
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }
    //podnies semafor, bo masz jedno mniej, czyli jedno wiecej miejxce
    pthread_mutex_unlock(&operations_mutex);
    printf("exiting critical section remove\n");
    printf("Element not found!\n");
    return 0;
}

void setSize(TQueue *queue, int *newSize) { //dok semfaory
    printf("entering critical section maxSize\n");
    pthread_mutex_lock(&operations_mutex);
    printf("in maxSize\n");
    if (newSize < queue->size) {
        message* curr = queue->startMsg;
        message* prev;
        //tutaj dać usuwanie wiadomosci //idk czy z pamieci tez maja byc usuwane
        for (int i = 0; i < queue->size - *newSize; i++) {
            prev->next = curr->next;
            curr = curr->next;
        }
        for (int i = 0; i < queue->maxSize - queue->size; i++) {
            //opuszczanie semafora do zera, bo jest wypełnione na maxa
        }
        queue->size = *newSize;
    }
    else {
        for (int i = 0; i < *newSize - queue->maxSize; i++){

        }
        //podnies semafor newSize-maxSize, bo tyle jest wiecej miejsca
    }
    queue->maxSize = *newSize;
    pthread_mutex_unlock(&operations_mutex);
    printf("exiting critical section maxSize\n");
}