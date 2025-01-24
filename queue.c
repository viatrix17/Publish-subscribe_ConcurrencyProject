#include "queue.h"

void delMsg(TQueue *queue, Message *msg) {
    if (queue->msgList->head == msg) {
            queue->msgList->head = ((Message*)queue->msgList->head)->next; //zmienia na liscie
        }
        else { //usuwanie wiadomosci
            Message *prevMsg = queue->msgList->head;
            while (prevMsg->next != msg) {
                prevMsg = prevMsg->next;
            }
            prevMsg->next = prevMsg->next->next;
        }
        queue->msgList->size--;
        pthread_mutex_lock(queue->operation_mutex); //budzenie jakiegos watku, ktore chce dodac wiadomosc bo ta jest usunieta
        pthread_cond_signal(queue->full);  
        pthread_mutex_unlock(queue->operation_mutex);
}

TQueue* createQueue(int *size) { 
    printf("Creating the queue...\n");
    TQueue* queue = (TQueue*)malloc(sizeof(TQueue));
    if (queue == NULL) { 
        printf("Memory allocation failed.\n");
        return NULL;
    }

    queue->access_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (queue->access_mutex == NULL) { 
        printf("Memory allocation failed.\n");
        return NULL;
    }
    queue->operation_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (queue->operation_mutex == NULL) { 
        printf("Memory allocation failed.\n");
        return NULL;
    }
    queue->full = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    if (queue->full == NULL) { 
        printf("Memory allocation failed.\n");
        return NULL;
    }

    queue->msgList = (List*)malloc(sizeof(List));
    if (queue->msgList == NULL) { 
        printf("Memory allocation failed.\n");
        return NULL;
    }  
    queue->subList = (List*)malloc(sizeof(List));
    if (queue->subList == NULL) { 
        printf("Memory allocation failed.\n");
        return NULL;
    } 

    //printf("Memory allocated correctly.\n");
    pthread_mutex_init(queue->access_mutex, NULL);
    pthread_mutex_init(queue->operation_mutex, NULL);
    pthread_cond_init(queue->full, NULL);
    //printf("Mutexes initlized correctly.\n");
    queue->maxSize = *size;
    
    //allocating memory for the lists 
    queue->msgList->head = NULL;
    queue->msgList->tail = NULL;
    queue->subList->head = NULL;
    queue->subList->tail = NULL;
    queue->msgList->size = 0;
    queue->subList->size = 0;

    printf("The queue has been created.\n");
    return queue;
}

void destroyQueue(TQueue **queue) {
    if (queue == NULL)  {
        return; //jesli juz nic nie ma, to nie ma czego zwolnic
    }
    Message *currMsg = (*queue)->msgList->head, *tempMsg;
    Subscriber* currSub = (*queue)->subList->head, *tempSub;
    while (currMsg != NULL) {
        
        tempMsg = currMsg;
        currMsg = currMsg->next;
        free(tempMsg); //zwolnienie pamieci dla elementu
    }
    while (currSub != NULL) {
        tempSub = currSub;
        currSub = currSub->next;
        free(tempSub);
    }
    if ((*queue)->access_mutex != NULL) {
        pthread_mutex_destroy((*queue)->access_mutex);  // Destroy the mutex before freeing it
        free((*queue)->access_mutex);  // Free the allocated memory for the mutex
    }
    if ((*queue)->operation_mutex != NULL) {
        pthread_mutex_destroy((*queue)->operation_mutex);  
        free((*queue)->operation_mutex);  
    }
    if ((*queue)->full != NULL) {
        pthread_cond_destroy((*queue)->full);  
        free((*queue)->full);  
    }
    free(*queue);
    *queue = NULL;
}

void subscribe(TQueue *queue, pthread_t *thread) { //sprawdzanie na początku, czy już ten zasubkrybował, bo po co tworzy drugi raz 

    printf("Subscribing the queue...\n");
    pthread_mutex_lock(queue->access_mutex);
    Subscriber *currSub = queue->subList->head;
    printf("ok\n");
    if (currSub != NULL) {
        while (currSub->threadID != thread) {
            currSub = currSub->next;
        }
    }
    if (currSub == NULL) {
        
        Subscriber* newSubscriber = (Subscriber*)malloc(sizeof(Subscriber));
        if (newSubscriber == NULL) {
            printf("Memory allocation failed!\n");
            return;
        }
        newSubscriber->list_empty = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
        if (newSubscriber->list_empty == NULL) { 
            printf("Memory allocation failed.\n");
            return;
        }
        newSubscriber->threadID = thread;
        newSubscriber->next = NULL;
        newSubscriber->startReading = NULL;
        
        pthread_mutex_init(newSubscriber->list_empty, NULL);
        
        if(queue->msgList->head == NULL) {
            queue->subList->head = newSubscriber;
            queue->subList->tail = newSubscriber;
        }
        else {
            ((Subscriber*)queue->subList->tail)->next = newSubscriber; 
            queue->subList->tail = newSubscriber;
        }
        queue->subList->size++;
        pthread_mutex_unlock(queue->access_mutex);
    }
    else {
        pthread_mutex_unlock(queue->access_mutex);
        printf("This thread is already subscribing the queue. Exiting the function...\n");
    }
}

void unsubscribe(TQueue *queue, pthread_t *thread) { //dok usuwanie chyba trzeba zrobic
    pthread_mutex_lock(queue->access_mutex);
    if (queue == NULL || queue->msgList == NULL) {
        pthread_mutex_unlock(queue->access_mutex);
        printf("exiting critical section remove\n");
        return;
    }
    if (((Subscriber*)queue->subList->head)->threadID == thread) {
        queue->subList->head = ((Subscriber*)queue->subList->head)->next;
    }
    else {
        Subscriber* prev = queue->subList->head;
        Message* tempMsg;
        while (prev->next != NULL) {
            if (prev->next->threadID == thread) {
                pthread_mutex_destroy(prev->next->list_empty);
                tempMsg = prev->next->startReading;
                //wiadomosci, ktore maja go na swojej liscie maja o jedno mniej do przeczytania
                while (tempMsg != NULL) {
                    tempMsg->readCount--;
                    if (tempMsg->readCount == 0) {
                        delMsg(queue, tempMsg);
                    }
                    tempMsg = tempMsg->next;
                }
                prev->next = prev->next->next;
                queue->subList->size--;
                pthread_mutex_unlock(queue->access_mutex);
                printf("Unsubscribed!\n");
                return;
            }
            prev = prev->next;
        }
    }
    pthread_mutex_unlock(queue->access_mutex);
    printf("exiting critical section remove\n");
    printf("Subsriber not found!\n");
    return;
}

void addMsg(TQueue *queue, void *msg) {

    pthread_mutex_lock(queue->operation_mutex);
    while (queue->msgList->size == queue->maxSize) { //block, obudzi się, jak bedzie usunieta wiadomosc
        pthread_cond_wait(queue->full, queue->operation_mutex);
    }
    pthread_mutex_unlock(queue->operation_mutex);

    Message* newMsg = (Message*)malloc(sizeof(Message)); //owning the memory it's pointing to
    if (newMsg == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
    //printf("entering critical section addMsg\n");
    pthread_mutex_lock(queue->access_mutex);
    if (queue->subList->size == 0) {
        printf("No active subscribers - exiting funciton...\n");
        pthread_mutex_unlock(queue->access_mutex);
        return;
    }
    newMsg->content = msg;
    newMsg->next = NULL;
    newMsg->readCount = queue->subList->size;
    newMsg->firstSub = queue->subList->head;

    //przypadek kiedy nie ma zadnych wiadomosci w kolejce
    if (queue->msgList->head == NULL) {
        queue->msgList->head = newMsg;
        queue->msgList->tail = newMsg;
    }
    else { //dodajemy na koniec
        ((Message*)queue->msgList->tail)->next = newMsg;
        queue->msgList->tail = newMsg;
    }
    queue->msgList->size++;
    //tutaj wszyscy subskrybenci mają jedną więcej wiadomość do przeczytania
    Subscriber* currSub = queue->subList->head;
    while(currSub != NULL) {
        //watek nie mial zadnych wiadomosci do przeczytania
        if (currSub->startReading == NULL) {
            currSub->startReading = newMsg;
        }
        //watek mial juz jakies do przeczytania, wiec nie to samo znajdzie, gdzie czytac
        currSub = currSub->next;
    }

    pthread_mutex_unlock(queue->access_mutex);
    printf("Message added\n");

    //printf("exiting critical section put\n");
}

void* getMsg(TQueue *queue, pthread_t *thread) { 
    //watek moze czytac wiadomosci wyslane po subskrypcji

    printf("enters critical section get\n");
    pthread_mutex_lock(queue->access_mutex);

    printf("in get\n");
    Subscriber* temp = queue->subList->head; //temp is a current thread
    while (temp->threadID != thread) {
        temp = temp->next;
    }
    if (temp == NULL) {
        printf("This thread is not subsrcibing this queue\n");
        pthread_mutex_unlock(queue->access_mutex);
        return NULL;
    }
    else if (temp->startReading == NULL) {
        printf("The list of messages for this subscriber is empty. Waiting...\n"); //musi byc blokujace uzyc zmiennych warunkowych
        pthread_mutex_lock(temp->list_empty);
        pthread_cond_wait(temp->empty, temp->list_empty);
        pthread_mutex_unlock(temp->list_empty); 
    }
    Message* receivedMsg = temp->startReading;
    //usuwanie pierwszej wiadomosci z listy "to read" w watku
    temp->startReading->readCount--;
    //sprawdzanie czy jest odczytana wiadomosc przez wszystkich
    if (temp->startReading->readCount == 0) {
        delMsg(queue, temp->startReading);
    }
    temp->startReading = temp->startReading->next; //przesuwamy o jedno dalej do czytania
    pthread_mutex_unlock(queue->access_mutex);
    printf("exiting critical section get\n");
    return receivedMsg;
}

void getAvailable(TQueue *queue, pthread_t *thread) {

    int count = 0;

    pthread_mutex_lock(queue->access_mutex);
    Subscriber* tempSub = queue->subList->head;
    while (tempSub->threadID != thread) {
        tempSub = tempSub->next;
    }
    if (tempSub == NULL) {
        printf("This thread doesn't subscribe this queue.\n");
        return;
    }
    else {
        Message* tempMsg = tempSub->startReading;
        while (tempMsg != NULL) {
            count++;
            tempMsg = tempMsg->next;
        }
    }
    printf("The number of available messages for this thread is: %d\n", count);
    pthread_mutex_unlock(queue->access_mutex);
}

void removeMsg(TQueue *queue, void *msg) { 
    // bezwarunkowo usuwa wiadomosc
    printf("entering critical section remove\n");
    pthread_mutex_lock(queue->access_mutex);
    printf("in remove\n");
    if (queue == NULL || queue->msgList == NULL) {
        pthread_mutex_unlock(queue->access_mutex);
        printf("exiting critical section remove\n");
        return;
    }
    if (((Message *)queue->msgList->head)->content == msg) {
        queue->msgList->head = ((Message *)queue->msgList->head)->next;
        pthread_mutex_lock(queue->operation_mutex);
        pthread_cond_signal(queue->full);  
        pthread_mutex_unlock(queue->operation_mutex);
        pthread_mutex_unlock(queue->access_mutex);
    }
    else {
        Message* prevMsg = queue->msgList->head;
        while (prevMsg->next != NULL) {
            if (prevMsg->next->content == msg) {
                prevMsg->next = prevMsg->next->next;
                printf("Element removed!\n");
                queue->maxSize--;
               //budzenie watku ktory chce dodac wiadomosc
                pthread_mutex_lock(queue->operation_mutex);
                pthread_cond_signal(queue->full);  
                pthread_mutex_unlock(queue->operation_mutex);
                pthread_mutex_unlock(queue->access_mutex);
                return;
            }
            prevMsg = prevMsg->next;
        }
    }
    printf("Element not found!\n");
    pthread_mutex_unlock(queue->access_mutex);
    printf("exiting critical section remove\n");
    return;
}

void setSize(TQueue *queue, int *newSize) { 
    printf("entering critical section maxSize\n");
    pthread_mutex_lock(queue->access_mutex);
    printf("in maxSize\n");
    if (*newSize < queue->maxSize) { //kiedy ma byc mniejsze
        Message* curr = queue->msgList->head;
        //usuwanie wiadomosci 
        for (int i = 0; i < queue->maxSize - *newSize; i++) {
            curr = curr->next;
        }
        queue->msgList->head = curr;
    }
    queue->maxSize = *newSize;
    pthread_mutex_unlock(queue->access_mutex);
    printf("exiting critical section maxSize\n");
}