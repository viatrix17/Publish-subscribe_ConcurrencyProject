#include "queue.h"

void deleteMsg(TQueue* queue, Message* currentMessage, Subscriber* startSub) {

    Subscriber* tempSubscriber = startSub;
    while (tempSubscriber != NULL) {
        if (tempSubscriber->startReading != NULL) {
            if (tempSubscriber->startReading == currentMessage) {
                tempSubscriber->startReading = currentMessage->next;
            } 
        }
        tempSubscriber = tempSubscriber->next;
    }

    //the message content should be freed
    Message* tempMessage = queue->msgList->head;
    Message* previousMessage = NULL;
    while (tempMessage != NULL) {
        if (tempMessage == currentMessage) {
            if (previousMessage == NULL) {
                queue->msgList->head = tempMessage->next;
            }
            else {
                previousMessage->next = tempMessage->next;
            }
            free(tempMessage);
            // preventing dangling references
            tempMessage = NULL; 
            break;
        }
        previousMessage = previousMessage->next;
        tempMessage = tempMessage->next;
    }
    queue->msgList->size--;

    pthread_cond_signal(queue->full);

}

void findSub(TQueue* queue, pthread_t thread) {
    Subscriber* currentSub = queue->subList->head;
    while (currentSub != NULL) {
        printf("%lu\t", currentSub->threadID);
        if (pthread_equal(currentSub->threadID, thread)) {
            break; 
        }
        currentSub = currentSub->next;
    }   
}

TQueue* createQueue(int size) { 

    // printf("Creating the queue...\n");
    // memory allocation for the queue
    TQueue* queue = (TQueue*)malloc(sizeof(TQueue));
    if (queue == NULL) { 
        //perror("Memory allocation failed.\n");
        return NULL;
    }
    // memory allocation for the mutexes and the conditional variables
    queue->access_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (queue->access_mutex == NULL) { 
        //perror("Memory allocation failed.\n");
        return NULL;
    }
    queue->full = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    if (queue->full == NULL) { 
        //perror("Memory allocation failed.\n");
        return NULL;
    }
    queue->empty = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    if (queue->empty == NULL) { 
        //perror("Memory allocation failed.\n");
        return NULL;
    }
    // memory allocation for the list of messages and the list of subscirbers
    queue->msgList = (List*)malloc(sizeof(List));
    if (queue->msgList == NULL) { 
        //perror("Memory allocation failed.\n");
        return NULL;
    }  
    queue->subList = (List*)malloc(sizeof(List));
    if (queue->subList == NULL) { 
        //perror("Memory allocation failed.\n");
        return NULL;
    } 
    // initialization of the mutexes and the conditional variable
    pthread_mutex_init(queue->access_mutex, NULL);
    pthread_cond_init(queue->full, NULL);
    pthread_cond_init(queue->empty, NULL);

    // initialization of the lists of stored data
    queue->maxSize = size;
    queue->msgList->head = NULL;
    queue->msgList->tail = NULL;
    queue->subList->head = NULL;
    queue->subList->tail = NULL;
    queue->msgList->size = 0;
    queue->subList->size = 0;

    // printf("The queue has been created.\n");
    return queue;
}

void destroyQueue(TQueue* queue) { //hmmm jak to zrobic na 

    //printf("Destroying queue...\n");
    if (queue == NULL)  {
        //printf("Nothing to destroy.\n");
        return; 
    }
    Message* currMsg = queue->msgList->head, *tempMsg;
    Subscriber* currSub = queue->subList->head, *tempSub;
    while (currMsg != NULL) {
        tempMsg = currMsg;
        currMsg = currMsg->next;
        free(tempMsg); 
    }
    while (currSub != NULL) {
        tempSub = currSub;
        currSub = currSub->next;
        free(tempSub);
    }
    if (queue->access_mutex != NULL) {
        pthread_mutex_destroy(queue->access_mutex);  
        free(queue->access_mutex);  
    }
    if (queue->full != NULL) {
        pthread_cond_destroy(queue->full);  
        free(queue->full);  
    }
    if (queue->empty != NULL) {
        pthread_cond_destroy(queue->empty);  
        free(queue->empty);  
    }
    free(queue);
    queue = NULL;
    //printf("Destroyed successfully\n");
}

void subscribe(TQueue* queue, pthread_t thread) { 

    if (queue == NULL) {
        return;
    }

    pthread_mutex_lock(queue->access_mutex);

    printf("Subscribing the queue...\t%lu\n", (unsigned long)thread);

    Subscriber* currentSub = queue->subList->head;
    while (currentSub != NULL) {
        if (pthread_equal(currentSub->threadID, thread)) {
            printf("This thread is already subscribing this queue. Exiting the function...\n");
            pthread_mutex_unlock(queue->access_mutex);
            return; 
        }
        currentSub = currentSub->next;
    }

    if (currentSub == NULL) {

    // memory allocation for the new subscriber
    Subscriber* newSubscriber = (Subscriber*)malloc(sizeof(Subscriber));
    if (newSubscriber == NULL) {
        pthread_mutex_unlock(queue->access_mutex);
        //perror("Memory allocation failed!\n");
        return;
    }
    // initialization of the new subscriber
    newSubscriber->threadID = thread;
    newSubscriber->next = NULL;
    newSubscriber->startReading = NULL;
    newSubscriber->msgCount = 0;
        
    // adding the new subscriber to the list of subscribers
    if (queue->subList->head == NULL) {
        queue->subList->head = newSubscriber;
        queue->subList->tail = newSubscriber;
    }
    else {
        ((Subscriber*)queue->subList->tail)->next = newSubscriber; 
        queue->subList->tail = newSubscriber; 
    }
    queue->subList->size++;
    printf("Subscribed!\n");
    }

    pthread_mutex_unlock(queue->access_mutex);
    
}

void unsubscribe(TQueue* queue, pthread_t thread) { 
    
    if (queue == NULL) {
        return;
    }

    pthread_mutex_lock(queue->access_mutex);
    printf("Unsubscribing the queue...\t%lu\n", (unsigned long)thread);

    if (queue->subList->head == NULL) {
        printf("No subscribers to unsubscribe. Exiting...\n");
        pthread_mutex_unlock(queue->access_mutex);
        return; 
    }

    Subscriber* currentSub = (Subscriber*) queue->subList->head;
    Subscriber* previousSub = NULL;
    Subscriber* startSub = (Subscriber*) queue->subList->head;

    Message* currentMessage;

    while (currentSub != NULL) {

        if (pthread_equal(currentSub->threadID, thread)) {
            currentMessage = currentSub->startReading;
             // Now unlink this subscriber from the list
            if (previousSub == NULL) {
                // We're removing the first element
                startSub = currentSub;
                queue->subList->head = currentSub->next;
            } 
            else {
                // Bypass the current subscriber
                previousSub->next = currentSub->next;
            }

            if (queue->subList->tail == currentSub) {
                queue->subList->tail = previousSub;
            }
            break;
        }
        previousSub = currentSub;
        currentSub = currentSub->next;
    }
    
    if (currentSub == NULL) {
        printf("Subscriber not found\n");
        pthread_mutex_unlock(queue->access_mutex);
        return;
    }
    //decrementing readCount of each message in subscriber's message list 
    while (currentMessage != NULL) {
        if (currentMessage->content == NULL) {
            printf("Message content is null. Invalid state. Exiting the program...\n");
            exit(1);
        } 
        if (currentMessage->readCount < 0) {
            printf("Message's list of subscribers is invalid. Exiting the program...\n");
            exit(1);
        }
        //the message has been read by all its subscribers - it should be removed
        if (currentMessage->readCount > 0) {
            currentMessage->readCount--;
        }
        Message* nextMessage = currentMessage->next;
        if (currentMessage->readCount == 0) {
            //finding subscribers with this message on their message lists
            deleteMsg(queue, currentMessage, startSub); 
        }

        currentMessage = nextMessage;
    }
    
    printf("Unsubscribed\n");
    queue->subList->size--;
    free(currentSub);
    currentSub = NULL;
    pthread_mutex_unlock(queue->access_mutex);
}

void addMsg(TQueue* queue, void* msg) {
    
    if (queue == NULL) {
        return;
    }

    if (msg == NULL) {
        printf("Given message content is null. Invalid state. Exiting the program...\n");
        exit(1);
    } 

    pthread_mutex_lock(queue->access_mutex);
    printf("Adding a message...\t%s\n", (char*)msg);


    // blocking behaviour when the queue is full
    while (queue->msgList->size >= queue->maxSize) { 
        printf("Queue size exceeded. Waiting...\n");
        pthread_cond_wait(queue->full, queue->access_mutex);
        printf("Checking for free space...\n");
    }
    if (queue->subList->size == 0) {
        pthread_mutex_unlock(queue->access_mutex);
        printf("No active subscribers - exiting function...\n");
        return;
    }

    Message* newMsg = (Message*)malloc(sizeof(Message)); 
    if (newMsg == NULL) {
        pthread_mutex_unlock(queue->access_mutex);
        // perror("Memory allocation failed!\n");
        return;
    }
    newMsg->content = msg;
    newMsg->next = NULL;
    newMsg->readCount = queue->subList->size;
    // newMsg->firstSub = queue->subList->head;

    if (queue->msgList->head == NULL) {
        queue->msgList->head = newMsg;
        queue->msgList->tail = newMsg;
    }
    else { 
        ((Message*)queue->msgList->tail)->next = newMsg;
        queue->msgList->tail = newMsg;
        //printf("new tail%s\n", (char*)((Message*)queue->msgList->tail)->content);
    }
    queue->msgList->size++;
    
    // updating the list of messages for all active subscribers
    Subscriber* currSub = queue->subList->head;
    while(currSub != NULL) {
        if (currSub->msgCount == 0) { 
            currSub->startReading = newMsg;
        }
        currSub->msgCount++;
        currSub = currSub->next;
    }
    // waking up the threads that are waiting for new messages
    pthread_cond_broadcast(queue->empty); 

    printf("Message added\n");
    pthread_mutex_unlock(queue->access_mutex);
}

void* getMsg(TQueue* queue, pthread_t thread) { 

    pthread_mutex_lock(queue->access_mutex);
    printf("Getting a message for thread %lu...\n", (unsigned long)thread);

    Subscriber* currentSub = queue->subList->head;
    while (currentSub != NULL) {
        if (pthread_equal(currentSub->threadID, thread)) {
            break; 
        }
        currentSub = currentSub->next;
    }

    if (currentSub == NULL) {
        printf("This thread is not subscribing this queue.\n");
        pthread_mutex_unlock(queue->access_mutex);
        return NULL;
    }
    // blocking behaviour when the list of messages is empty
    while (currentSub->startReading == NULL) {
        printf("The list of messages for thread %lu is empty. Waiting...\n", (unsigned long)thread); 
        pthread_cond_wait(queue->empty, queue->access_mutex);
        printf("Checking for new messages...\n");
    }
    char* receivedMsg = currentSub->startReading->content;
    currentSub->startReading->readCount--;
    Message* tempMessage = currentSub->startReading;
    currentSub->startReading = currentSub->startReading->next; 

    // checking if the received message might be deleted
    if (tempMessage->readCount == 0) {
        deleteMsg(queue, tempMessage, queue->subList->head);
    }
    currentSub->msgCount--;
    // waking a thread that is waiting for free space in the queue
    pthread_cond_signal(queue->full);  
    printf("Message received!\n");
    pthread_mutex_unlock(queue->access_mutex);

    return receivedMsg;
}

int getAvailable(TQueue* queue, pthread_t thread) {

    int count = -1;

    pthread_mutex_lock(queue->access_mutex);
    // printf("Get available\n");
    Subscriber* currentSub = queue->subList->head;
    while (currentSub != NULL) {
        printf("%lu\t", currentSub->threadID);
        if (pthread_equal(currentSub->threadID, thread)) {
            break; 
        }
        currentSub = currentSub->next;
    }
    if (currentSub == NULL) {
        pthread_mutex_unlock(queue->access_mutex);
        // printf("Thread %lu doesn't subscribe this queue.\n", thread);
        return count;
    }
    count = currentSub->msgCount;
    // printf("Count returned\n");
    pthread_mutex_unlock(queue->access_mutex);
    return count;
}

void removeMsg(TQueue* queue, void* msg) { 

    if (queue == NULL) {
        return;
    }
    pthread_mutex_lock(queue->access_mutex);
    printf("Removing a message...\t%s\n", (char*)msg);
    if (queue->msgList->head == NULL) {
        pthread_mutex_unlock(queue->access_mutex);
        printf("Element not found - the queue is empty!\n");
        return;
    }

    Message* currentMessage = (Message*) queue->msgList->head;

    Message* previousMessage = NULL;

    Message* startMessage = (Message*) queue->subList->head;


    while (currentMessage != NULL) {
        if (currentMessage->content == msg) {
             // Now unlink this message from the list
            if (previousMessage == NULL) {
                // We're removing the first element
                startMessage = currentMessage;
                queue->msgList->head = currentMessage->next;
            } 
            else {
                // Bypass the current message
                previousMessage->next = currentMessage->next;
            }

            if (queue->subList->tail == currentMessage) {
                queue->subList->tail = previousMessage;
            }
            break;
        }
        previousMessage = currentMessage;
        currentMessage = currentMessage->next;
    }

    if (currentMessage == NULL) {
        printf("Message not found\n");
        pthread_mutex_unlock(queue->access_mutex);
        return;
    }

    // checking for each subscriber if the removed message is the first message on its list of messages 
    Subscriber* tempSubscriber = queue->subList->head;
    while (tempSubscriber != NULL) {
        if (tempSubscriber->startReading != NULL) {
            if (tempSubscriber->startReading == currentMessage) {
                tempSubscriber->startReading = currentMessage->next;
            } 
        }
        tempSubscriber = tempSubscriber->next;
    }
    free(currentMessage);
    currentMessage = NULL;
    printf("Message removed!\n");
    pthread_cond_signal(queue->full);
    pthread_mutex_unlock(queue->access_mutex);
}

void setSize(TQueue* queue, int newSize) { 
    
    pthread_mutex_lock(queue->access_mutex);
    // printf("Setting new size...\n");
    int currSize = queue->msgList->size;
    if (newSize < currSize) { 
        Subscriber* tempSub;
        // printf("New size is smaller than the current queue size\n");
        Message* curr = queue->msgList->head;
        // removing first n messages (calculated based on new size)
        for (int i = 0; i < currSize - newSize; i++) {
            // removing messages from subscribers' lists of messages
            tempSub = queue->subList->head;
            while (tempSub != NULL) {
                if (tempSub->startReading->content == curr->content) {
                    tempSub->msgCount--;
                    tempSub->startReading = tempSub->startReading->next;
                }
                tempSub = tempSub->next;
            }

            curr = curr->next;
            queue->msgList->size--;
        }
        queue->msgList->head = curr;
    }
    // waking up the threads that are waiting for free space in the queue
    else {
        pthread_cond_broadcast(queue->full);
    }
    queue->maxSize = newSize;
    // printf("New size has been set successfully.\n");
    pthread_mutex_unlock(queue->access_mutex);
}