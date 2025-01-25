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
        pthread_cond_signal(queue->block_operation);  
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
    queue->block_operation = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    if (queue->block_operation == NULL) { 
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
    pthread_cond_init(queue->block_operation, NULL);
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
    printf("Destroying queue...\n");
    if (*queue == NULL)  {
        printf("Nothing to destroy.\n");
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
    if ((*queue)->block_operation != NULL) {
        pthread_cond_destroy((*queue)->block_operation);  
        free((*queue)->block_operation);  
    }
    free(*queue);
    *queue = NULL;
    printf("Destroyed successfully\n");
}

void subscribe(TQueue *queue, pthread_t *thread) { 

    printf("Subscribing the queue...\n");
    pthread_mutex_lock(queue->access_mutex);
    Subscriber *currSub = queue->subList->head;
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
        newSubscriber->threadID = thread;
        newSubscriber->next = NULL;
        newSubscriber->startReading = NULL;
        
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
        printf("Subscribed!\n");
    }
    else {
        pthread_mutex_unlock(queue->access_mutex);
        printf("This thread is already subscribing the queue. Exiting the function...\n");
    }
}

void unsubscribe(TQueue *queue, pthread_t *thread) { 
    
    pthread_mutex_lock(queue->access_mutex);
    if (queue == NULL || queue->msgList->head == NULL) {
        pthread_mutex_unlock(queue->access_mutex);
        //printf("exiting critical section remove\n");
        return;
    }

    Message* tempMsg;
    if (((Subscriber*)queue->subList->head)->threadID == thread) {
        //DO NAPISANIA LADNIE
        tempMsg = ((Subscriber*)queue->subList->head)->startReading;
                //wiadomosci, ktore maja go na swojej liscie maja o jedno mniej do przeczytania
                while (tempMsg != NULL) {
                    tempMsg->readCount--;
                    if (tempMsg->readCount == 0) {
                        delMsg(queue, tempMsg);
                    }
                    tempMsg = tempMsg->next;
                }
                queue->subList->head = ((Subscriber*)queue->subList->head)->next;
                queue->subList->size--;
                pthread_mutex_unlock(queue->access_mutex);
                printf("Unsubscribed!\n");
                return;
    }
    else {
        Subscriber* prev = queue->subList->head;
        while (prev->next != NULL) {
            if (prev->next->threadID == thread) {
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
    //printf("exiting critical section remove\n");
    printf("Subscriber not found!\n");
    return;
}

void addMsg(TQueue *queue, void *msg) {

    pthread_mutex_lock(queue->access_mutex);
    pthread_mutex_lock(queue->operation_mutex);
    while (queue->msgList->size == queue->maxSize) { //block, obudzi się, jak bedzie usunieta wiadomosc
        printf("Queue size exceeded. Waiting...\n");
        pthread_mutex_unlock(queue->access_mutex);
        pthread_cond_wait(queue->block_operation, queue->operation_mutex);
        //po wybudzeniu sprawdza i ewentualnie blokuje znowu; jak wyjdzie z petli to juz ma locked czyli tak jak gdyby nie bylo tego sprawdzania warunku
        pthread_mutex_lock(queue->access_mutex);
    }
    pthread_mutex_unlock(queue->operation_mutex);

    Message* newMsg = (Message*)malloc(sizeof(Message)); //owning the memory it's pointing to
    if (newMsg == NULL) {
        printf("Memory allocation failed!\n");
        return;
    }
    //printf("entering critical section addMsg\n");
    if (queue->subList->size == 0) {
        printf("No active subscribers - exiting function...\n");
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
        //watek mial juz jakies do przeczytania, to samo znajdzie, gdzie czytac
        currSub = currSub->next;
    }
    pthread_mutex_lock(queue->operation_mutex);
    pthread_cond_broadcast(queue->block_operation);  //broadcast na wszystkie, bo wszystkie watki na wiadomosc czekajace sie obudzą, a te zablokowane na przepełnieniu ewentualnie znowu wejdą do pętli
    pthread_mutex_unlock(queue->operation_mutex);

    pthread_mutex_unlock(queue->access_mutex);
    printf("Message added\n");
}

void* getMsg(TQueue *queue, pthread_t *thread) { 
    //watek moze czytac wiadomosci wyslane po subskrypcji

    //printf("enters critical section get\n");
    pthread_mutex_lock(queue->access_mutex);

    Subscriber* temp = queue->subList->head; //temp is a current thread
    while (temp->threadID != thread) {
        temp = temp->next;
    }
    if (temp == NULL) {
        printf("This thread is not subsrcibing this queue\n");
        pthread_mutex_unlock(queue->access_mutex);
        return NULL;
    }
    //tu jest locked access mutex na tym etapie
    pthread_mutex_lock(queue->operation_mutex);
    while(temp->startReading == NULL) {
        printf("The list of messages for this subscriber is empty. Waiting...\n"); //musi byc blokujace uzyc zmiennych warunkowych
        pthread_mutex_unlock(queue->access_mutex); //czeka a inne mogą sobie robic co chcą
        pthread_cond_wait(queue->block_operation, queue->operation_mutex);
        pthread_mutex_lock(queue->access_mutex); //jak wyjdzie z petli to bedzie normalnie robic
    }
    pthread_mutex_unlock(queue->operation_mutex); 
    Message* receivedMsg = temp->startReading;
    //usuwanie pierwszej wiadomosci z listy "to read" w watku
    temp->startReading->readCount--;
    //sprawdzanie czy jest odczytana wiadomosc przez wszystkich
    if (temp->startReading->readCount == 0) {
        delMsg(queue, temp->startReading);
    }
    temp->startReading = temp->startReading->next; //przesuwamy o jedno dalej do czytania
    pthread_mutex_unlock(queue->access_mutex);
    //printf("exiting critical section get\n");
    return receivedMsg;
}

int getAvailable(TQueue *queue, pthread_t *thread) {

    int count = 0;

    pthread_mutex_lock(queue->access_mutex);
    Subscriber* tempSub = queue->subList->head;
    while (tempSub->threadID != thread) {
        tempSub = tempSub->next;
    }
    if (tempSub == NULL) {
        printf("This thread doesn't subscribe this queue.\n");
        return count;
    }
    else {
        Message* tempMsg = tempSub->startReading;
        while (tempMsg != NULL) {
            count++;
            tempMsg = tempMsg->next;
        }
    }
    pthread_mutex_unlock(queue->access_mutex);
    return count;
}

void removeMsg(TQueue *queue, void *msg) { //zakładam, że żadna wiadomosc nie pojawia się dwa razy!!!
    // bezwarunkowo usuwa wiadomosc

    //printf("entering critical section remove\n");
    pthread_mutex_lock(queue->access_mutex);

    if (queue == NULL || queue->msgList->head == NULL) {
        pthread_mutex_unlock(queue->access_mutex);
        printf("Element not found!\n");
        return;
    }

    if (((Message *)queue->msgList->head)->content == msg) { //jesli ta wiaodmocs jest pierwsza na liscie, to przesunac heada
        printf("first\n");
        queue->msgList->head = ((Message *)queue->msgList->head)->next; 
        queue->msgList->size--;
    }
    else { //jesli ta wiadomosc jest pozniej, to ją znalezc i usunac
        Message* prevMsg = queue->msgList->head;
        while (prevMsg->next != NULL) {
            if (prevMsg->next->content == msg) {
                prevMsg->next = prevMsg->next->next;
                printf("Element removed!\n");
                queue->maxSize--;
                break;  
            }
            prevMsg = prevMsg->next;
        }
        if (prevMsg->next == NULL) { //jezeli nie znalazło
            printf("Element not found!\n");
            pthread_mutex_unlock(queue->access_mutex);
            return;
        }
    }
    //sprawdzic we wszystkich subskrybentach, czy jest na poczatku
    Subscriber* tempSub = queue->subList->head;
    while (tempSub != NULL) {
        if (tempSub->startReading->content == msg) {
            tempSub->startReading = tempSub->startReading->next; //jak jest na poczatku, to przesunac heada na nastepna
        }
        tempSub = tempSub->next;
    }
    //odblokowac zablokowane przez cond var wątki, ale tylko pojedyncze, bo pojedyncze miejsce się zwalnia
    pthread_mutex_lock(queue->operation_mutex);
    pthread_cond_signal(queue->block_operation);  
    pthread_mutex_unlock(queue->operation_mutex);
    pthread_mutex_unlock(queue->access_mutex);
    printf("Element removed successfully!\n");
    return;
}

void setSize(TQueue *queue, int *newSize) { 
    
    pthread_mutex_lock(queue->access_mutex);
    printf("Setting new size...\n");
    int currSize = queue->msgList->size;
    if (*newSize < currSize) { //kiedy ma byc mniejsze
        Subscriber* tempSub;
        printf("New size is smaller than the current queue size\n");
        Message* curr = queue->msgList->head;
        //usuwanie wiadomosci 
        for (int i = 0; i < currSize - *newSize; i++) {
            //usuwanie u subskrybentow
            tempSub = queue->subList->head;
            while (tempSub != NULL) {
                if (tempSub->startReading->content == curr->content) {
                    tempSub->startReading = tempSub->startReading->next; //jak jest na poczatku, to przesunac heada na nastepna
                }
                tempSub = tempSub->next;
            }

            curr = curr->next;
            queue->msgList->size--;
        }
        queue->msgList->head = curr;
    }
    pthread_mutex_lock(queue->operation_mutex);
    pthread_cond_broadcast(queue->block_operation);
    pthread_mutex_unlock(queue->operation_mutex);
    queue->maxSize = *newSize;
    printf("New size has been set successfully.\n");
    pthread_mutex_unlock(queue->access_mutex);
}