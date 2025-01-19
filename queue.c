#include "queue.h"

//ewentualnie jeszcze zrobic lastmessage zeby tego nie iterowac zcaly czas


TQueue* createQueue(int *size) { 
    TQueue* queue = (TQueue*)malloc(sizeof(TQueue));
    if (queue == NULL) { 
        printf("Memory allocation failed.\n");
        return NULL;
    }

    pthread_mutex_init(&queue->operations_mutex, NULL);
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
    pthread_mutex_destroy(&(*queue)->operations_mutex);
    free(*queue);
    *queue = NULL;
}

void subscribe(TQueue *queue, pthread_t *thread) {

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
    
    pthread_mutex_lock(&queue->operations_mutex);
    if(queue->firstSub == NULL) {
        queue->firstSub = newSubscriber;
    }
    else {
        queue->lastSub->next = newSubscriber; 
        queue->lastSub = newSubscriber;
    }
    pthread_mutex_unlock(&queue->operations_mutex);
}

void unsubscribe(TQueue *queue, pthread_t *thread) { //dok
    pthread_mutex_lock(&queue->operations_mutex);
    if (queue == NULL || queue->firstSub == NULL) {
        pthread_mutex_unlock(&queue->operations_mutex);
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
    pthread_mutex_unlock(&queue->operations_mutex);
    printf("exiting critical section remove\n");
    printf("Element not found!\n");
    return 0;
}

void addMsg(TQueue *queue, void *msg) { //dodac semafor
    //sem_wait(size_sem); //SEMAFOR DODAC po opusczeniu zamka, bo inaczej dwa mogą opuscic dodajc wiadomosc
    printf("in put\n");
    Message* newMsg = (Message*)malloc(sizeof(Message)); //owning the memory it's pointing to
    if (newMsg == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
    printf("entering critical section put\n");
    pthread_mutex_lock(&queue->operations_mutex);
    if (queue->subCount == 0) {
        //czy to legalne, że po prostu jej nie dodam, bo po prostu muszę ją i tak usunąć XD
        pthread_mutex_unlock(&queue->operations_mutex);
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

    pthread_mutex_unlock(&queue->operations_mutex);
    printf("exiting critical section put\n");
}

void getMsg(TQueue *queue, pthread_t *thread) { 
    //watek moze czytac wiadomosci wyslane po subskrypcji

    printf("enters critical section get\n");
    pthread_mutex_lock(&queue->operations_mutex);

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
        ///
        //
        //zmienne warunkowe uga buga; 
    }
    //usuwanie pierwszej wiadomosci z listy to read w watku
    Message* currMsg, *prevMsg;
    temp->messages->head = temp->messages->head->next; //nie ma zwalniania pamieci, bo to ten wskaznik z głownej kolejki
    temp->messages->size--;
    queue->startMsg->read++;
    //sprawdzanie czy jest odczytana wiadomosc
    if (currMsg->read == currMsg->subscribers->size) {
        Message* tempMsg = queue->startMsg;
        while (tempMsg != currMsg) {
            prevMsg = tempMsg;
            tempMsg = tempMsg->next;
        }
        if (tempMsg != NULL) {
            prevMsg->next = tempMsg->next;
            //usuwa jedną wiadomośc podnieść SEMAFOR 
        }   
    }
    pthread_mutex_unlock(&queue->operations_mutex);
    printf("exiting critical section get\n");
}

void getAvailable(TQueue *queue, pthread_t *thread) {
    pthread_mutex_lock(&queue->operations_mutex);
    subscriber* tempSub = queue->firstSub;
    while (tempSub->threadID != thread) {
        tempSub = tempSub->next;
    }
    if (tempSub == NULL) {
        printf("This thread doesn't subscribe this queue.\n");
        return 1;
    }
    printf("The number of available messages for this thread is: %d\n", tempSub->messages->size);
    pthread_mutex_unlock(&queue->operations_mutex);
}

void removeMsg(TQueue *queue, void *msg) { //dodac semafor // bezwarunkowo usuwa wiadomosc
    printf("entering critical section remove\n");
    pthread_mutex_lock(&queue->operations_mutex);
    printf("in remove\n");
    if (queue == NULL || queue->startMsg == NULL) {
        pthread_mutex_unlock(&queue->operations_mutex);
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
            //podnies semafor, bo masz jedno mniej, czyli jedno wiecej miejxce
            pthread_mutex_unlock(&queue->operations_mutex);
            return;
        }
        prevMsg = currMsg;
        currMsg = currMsg->next;
    }
    printf("Element not found!\n");
    pthread_mutex_unlock(&queue->operations_mutex);
    printf("exiting critical section remove\n");
    return 1;
}

void setSize(TQueue *queue, int *newSize) { //dok semfaory
    printf("entering critical section maxSize\n");
    pthread_mutex_lock(&queue->operations_mutex);
    printf("in maxSize\n");
    if (newSize < queue->size) {
        Message* curr = queue->startMsg;
        Message* prev;
        //usuwanie wiadomosci 
        for (int i = 0; i < queue->size - *newSize; i++) {
            //prev->next = curr->next;
            curr = curr->next;
        }
        queue->startMsg = curr;
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
    pthread_mutex_unlock(&queue->operations_mutex);
    printf("exiting critical section maxSize\n");
}