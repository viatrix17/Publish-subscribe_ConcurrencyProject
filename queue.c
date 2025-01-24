#include "queue.h"

TQueue* createQueue(int *size) { 
    TQueue* queue = (TQueue*)malloc(sizeof(TQueue));
    if (queue == NULL) { 
        printf("Memory allocation failed.\n");
        return NULL;
    }

    pthread_mutex_init(&queue->access_mutex, NULL);
    pthread_mutex_init(&queue->operation_mutex, NULL);
    queue->maxSize = size;
    queue->startMsg = NULL;
    queue->lastMsg = NULL;
    queue->firstSub = NULL;
    queue->lastSub = NULL;
    queue->size = 0;
    return queue;
}

void destroyQueue(TQueue **queue) {
    if (queue == NULL)  {
        return; //jesli juz nic nie ma, to nie ma czego zwolnic
    }
    Message *currMsg = (*queue)->startMsg, *tempMsg;
    subscriber* currSub = (*queue)->firstSub, *tempSub;
    while (currMsg != NULL && currSub != NULL) {
        
        tempMsg = currMsg;
        tempSub = currSub;
        currMsg = currMsg->next;
        currSub = currSub->next;
        free(tempMsg); //zwolnienie pamieci dla elementu
        free(tempSub);
    }
    pthread_mutex_destroy(&(*queue)->access_mutex);
    free(*queue);
    *queue = NULL;
}

void subscribe(TQueue *queue, pthread_t *thread) { //sprawdzanie na początku, czy już ten zasubkrybował, bo po co tworzy drugi raz 

    subscriber* newSubscriber = (subscriber*)malloc(sizeof(subscriber));
    if (newSubscriber = NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
    newSubscriber->threadID = thread;
    newSubscriber->next = NULL;
    newSubscriber->messages->head = NULL;
    newSubscriber->messages->tail = NULL;
    newSubscriber->messages->size = 0;
    pthread_mutex_init(newSubscriber->empty, NULL);
    
    pthread_mutex_lock(&queue->access_mutex);
    if(queue->firstSub == NULL) {
        queue->firstSub = newSubscriber;
    }
    else {
        queue->lastSub->next = newSubscriber; 
        queue->lastSub = newSubscriber;
    }
    pthread_mutex_unlock(&queue->access_mutex);
}

void unsubscribe(TQueue *queue, pthread_t *thread) { //dok usuwanie chyba trzeba zrobic
    pthread_mutex_lock(&queue->access_mutex);
    if (queue == NULL || queue->firstSub == NULL) {
        pthread_mutex_unlock(&queue->access_mutex);
        printf("exiting critical section remove\n");
        return 1;
    }
    subscriber* curr = queue->firstSub;
    subscriber* prev;
    while (curr->next != NULL) {
        if (curr->threadID == thread) {
            pthread_mutex_destroy(&curr->empty);
            prev->next = curr->next;
            //usuwac jeszcze tego subskrybenta, wszystkie jego dane DODAĆ
            printf("Unsubscribed!\n");
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&queue->access_mutex);
    printf("exiting critical section remove\n");
    printf("Subsriber not found!\n");
    return 0;
}

void addMsg(TQueue *queue, void *msg) {
    
    printf("in put\n");
    
    pthread_mutex_lock(&queue->operation_mutex);
    while (queue->size != queue->maxSize) { //block, obudzi się, jak bedzie usunieta wiadomosc
        pthread_cond_wait(&queue->full, &queue->operation_mutex);
    }
    pthread_mutex_unlock(&queue->operation_mutex);

    Message* newMsg = (Message*)malloc(sizeof(Message)); //owning the memory it's pointing to
    if (newMsg == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
    printf("entering critical section put\n");
    pthread_mutex_lock(&queue->access_mutex);
    if (queue->subCount == 0) {
        //abort mission
        pthread_mutex_unlock(&queue->access_mutex);
        return;
    }
    newMsg->content = msg;
    newMsg->next = NULL;
    newMsg->subscribers->head = queue->firstSub;
    newMsg->subscribers->tail = queue->lastSub;
    newMsg->subscribers->size = queue->subCount;

    //przypadek kiedy nie ma zadnych wiadomosci w kolejce
    if (queue->startMsg == NULL) {
        queue->startMsg = newMsg;
        queue->lastMsg = newMsg;
    }
    else { //dodajemy na koniec
        queue->lastMsg->next = newMsg;
        queue->lastMsg = newMsg;
    }
    queue->size++;
    //tutaj wszyscy subskrybenci mają jedną więcej wiadomość do przeczytania
    subscriber* currSub = queue->firstSub;
    while(currSub != NULL) {
        //watek nie mial zadnych wiadomosci do przeczytania
        if (currSub->messages = NULL) {
            currSub->messages->head = newMsg;
            currSub->messages->tail = newMsg;
            currSub->messages->size = 1;
        }
        //watek mial juz jakies do przeczytania
        else {
            currSub->messages->tail->next = newMsg;
            currSub->messages->tail = newMsg;
            currSub->messages->size++;
        }
        currSub = currSub->next;
    }

    pthread_mutex_unlock(&queue->access_mutex);
    printf("exiting critical section put\n");
}

void getMsg(TQueue *queue, pthread_t *thread) { 
    //watek moze czytac wiadomosci wyslane po subskrypcji

    printf("enters critical section get\n");
    pthread_mutex_lock(&queue->access_mutex);

    printf("in get\n");
    subscriber* temp = queue->firstSub; //temp is a current thread
    while (temp->threadID != thread) {
        temp = temp->next;
    }
    if (temp == NULL) {
        return NULL;
        //watek nie subkrybuje
    }
    else if (temp->messages == NULL) {
        printf("the list for this sub is empty. Waiting...\n"); //musi byc blokujace uzyc zmiennych warunkowych
        pthread_mutex_lock(&temp->list_empty);
        pthread_cond_wait(&temp->empty, &temp->list_empty);
        pthread_mutex_unlock(&temp->list_empty); 
    }
    //usuwanie pierwszej wiadomosci z listy "to read" w watku
    Message* currMsg, *prevMsg;
    temp->messages->head = temp->messages->head->next; //nie ma zwalniania pamieci, bo to ten wskaznik z głownej kolejki
    temp->messages->size--;
    queue->startMsg->read++;
    //sprawdzanie czy jest odczytana wiadomosc przez wszystkich
    if (currMsg->read == currMsg->subscribers->size) {
        Message* tempMsg = queue->startMsg;
        while (tempMsg != currMsg) {
            prevMsg = tempMsg;
            tempMsg = tempMsg->next;
        }
        if (tempMsg != NULL) {
            prevMsg->next = tempMsg->next;
        }   
        queue->size--;
        pthread_mutex_lock(&queue->operation_mutex); //budzenie jakiegos watku, ktore chce dodac wiadomosc
        pthread_cond_signal(&queue->full);  
        pthread_mutex_unlock(&queue->operation_mutex);
    }
    pthread_mutex_unlock(&queue->access_mutex);
    printf("exiting critical section get\n");
}

void getAvailable(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->access_mutex);
    subscriber* tempSub = queue->firstSub;
    while (tempSub->threadID != thread) {
        tempSub = tempSub->next;
    }
    if (tempSub == NULL) {
        printf("This thread doesn't subscribe this queue.\n");
        return 1;
    }
    printf("The number of available messages for this thread is: %d\n", tempSub->messages->size);
    pthread_mutex_unlock(&queue->access_mutex);
}

void removeMsg(TQueue *queue, void *msg) { 
    // bezwarunkowo usuwa wiadomosc
    printf("entering critical section remove\n");
    pthread_mutex_lock(&queue->access_mutex);
    printf("in remove\n");
    if (queue == NULL || queue->startMsg == NULL) {
        pthread_mutex_unlock(&queue->access_mutex);
        printf("exiting critical section remove\n");
        return 1;
    }
    Message* currMsg = queue->startMsg, *prevMsg, *tempMsg, *prevTempMsg;
    while (currMsg->next != NULL) {
        if (currMsg->content == msg) {
            //usun te wiadomosc na wszystkich subskrybentach
            subscriber* tempSub = queue->firstSub; 
            while (tempSub != NULL) {
                tempMsg = tempSub->messages->head;
                while (msg != tempMsg->content) {
                    prevTempMsg = tempMsg;
                    tempMsg = tempMsg->next;
                }
                if (tempMsg != NULL) {
                    prevTempMsg->next = tempMsg->next; 
                }
            }
            prevMsg->next = currMsg->next;
            printf("Element removed!\n");
            queue->size--;
            //budzenie watku ktory chce dodac wiadomosc
            pthread_mutex_lock(&queue->operation_mutex);
            pthread_cond_signal(&queue->full);  
            pthread_mutex_unlock(&queue->operation_mutex);
            pthread_mutex_unlock(&queue->access_mutex);
            return;
        }
        prevMsg = currMsg;
        currMsg = currMsg->next;
    }
    printf("Element not found!\n");
    pthread_mutex_unlock(&queue->access_mutex);
    printf("exiting critical section remove\n");
    return 1;
}

void setSize(TQueue *queue, int *newSize) { //usuwanie wiadomosci dok
    printf("entering critical section maxSize\n");
    pthread_mutex_lock(&queue->access_mutex);
    printf("in maxSize\n");
    if (newSize < queue->size) {
        Message* curr = queue->startMsg;
        Message* prev;
        //usuwanie wiadomosci dok
        for (int i = 0; i < queue->size - *newSize; i++) {
            //prev->next = curr->next;
            curr = curr->next;
        }
        queue->startMsg = curr;
        queue->size = *newSize;
    }
    queue->maxSize = *newSize;
    pthread_mutex_unlock(&queue->access_mutex);
    printf("exiting critical section maxSize\n");
}