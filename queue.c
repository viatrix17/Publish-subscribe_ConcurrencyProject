#include "queue.h"

void delMsg(TQueue* queue, Message* msg) {
    if (queue->msgList->head == msg) {
            queue->msgList->head = ((Message*)queue->msgList->head)->next;
    }
    else { 
        Message* prevMsg = queue->msgList->head;
        while (prevMsg->next != msg) {
            prevMsg = prevMsg->next;
        }
        prevMsg->next = prevMsg->next->next;
    }
    queue->msgList->size--;
    //waking a thread that is waiting for freeing up space in the queue
    pthread_mutex_lock(queue->operation_mutex); 
    pthread_cond_signal(queue->block_operation);  
    pthread_mutex_unlock(queue->operation_mutex);
}

void checkMsg(TQueue* queue, Message* msg) {
    while (msg != NULL) {
        msg->readCount--;
        if (msg->readCount == 0) {
            delMsg(queue, msg);
        }
        msg = msg->next;
    }
}

TQueue* createQueue(int* size) { 

   // printf("Creating the queue...\n");
    // memory allocation for the queue
    TQueue* queue = (TQueue*)malloc(sizeof(TQueue));
    if (queue == NULL) { 
        //perror("Memory allocation failed.\n");
        return NULL;
    }
    // memory allocation for the mutexes and the conditional variable
    queue->access_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (queue->access_mutex == NULL) { 
        //perror("Memory allocation failed.\n");
        return NULL;
    }
    queue->operation_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (queue->operation_mutex == NULL) { 
        //perror("Memory allocation failed.\n");
        return NULL;
    }
    queue->block_operation = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    if (queue->block_operation == NULL) { 
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
    pthread_mutex_init(queue->operation_mutex, NULL);
    pthread_cond_init(queue->block_operation, NULL);

    // initialization of the lists of stored data
    queue->maxSize = *size;
    queue->msgList->head = NULL;
    queue->msgList->tail = NULL;
    queue->subList->head = NULL;
    queue->subList->tail = NULL;
    queue->msgList->size = 0;
    queue->subList->size = 0;

    //printf("The queue has been created.\n");
    return queue;
}

void destroyQueue(TQueue** queue) {

    //printf("Destroying queue...\n");
    if (*queue == NULL)  {
        //printf("Nothing to destroy.\n");
        return; 
    }
    Message* currMsg = (*queue)->msgList->head, *tempMsg;
    Subscriber* currSub = (*queue)->subList->head, *tempSub;
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
    if ((*queue)->access_mutex != NULL) {
        pthread_mutex_destroy((*queue)->access_mutex);  
        free((*queue)->access_mutex);  
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
    //printf("Destroyed successfully\n");
}

void subscribe(TQueue* queue, pthread_t* thread) { 

    //printf("Subscribing the queue...\n");
    pthread_mutex_lock(queue->access_mutex);
    Subscriber *currSub = queue->subList->head;
    while (currSub != NULL) {
        if (currSub->threadID == thread) {
            pthread_mutex_unlock(queue->access_mutex);
            //printf("This thread is already subscribing this queue. Exiting the function...\n");
            return;
        }
        currSub = currSub->next;
    }
    if (currSub == NULL) {
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
        
        // adding the new subscriber to the list of subscribers
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
        //printf("Subscribed!\n");
    }
}

void unsubscribe(TQueue* queue, pthread_t* thread) { 
    
    pthread_mutex_lock(queue->access_mutex);
    if (queue == NULL || queue->msgList->head == NULL) {
        pthread_mutex_unlock(queue->access_mutex);
        //printf("Nothing to unsubscribe. Exiting...\n");
        return;
    }

    Message* tempMsg;
    if (((Subscriber*)queue->subList->head)->threadID == thread) {
        tempMsg = ((Subscriber*)queue->subList->head)->startReading;
                // updating readCount for the messages that the subscriber was supposed to receive
                checkMsg(queue, tempMsg);
                queue->subList->head = ((Subscriber*)queue->subList->head)->next;
                queue->subList->size--;
                pthread_mutex_unlock(queue->access_mutex);
                //printf("Unsubscribed!\n");
                return;
    }
    else {
        Subscriber* prev = queue->subList->head;
        while (prev->next != NULL) {
            if (prev->next->threadID == thread) {
                tempMsg = prev->next->startReading;
                checkMsg(queue, tempMsg);
                prev->next = prev->next->next;
                queue->subList->size--;
                pthread_mutex_unlock(queue->access_mutex);
                //printf("Unsubscribed!\n");
                return;
            }
            prev = prev->next;
        }
    }
    pthread_mutex_unlock(queue->access_mutex);
    //printf("Subscriber not found!\n");
    return;
}

void addMsg(TQueue* queue, void* msg) {
    
    //printf("Adding a message...\n");
    pthread_mutex_lock(queue->access_mutex);
    // blocking behaviour when the queue is full
    pthread_mutex_lock(queue->operation_mutex);
    while (queue->msgList->size == queue->maxSize) { 
        //printf("Queue size exceeded. Waiting...\n");
        pthread_mutex_unlock(queue->access_mutex);
        pthread_cond_wait(queue->block_operation, queue->operation_mutex);
        pthread_mutex_lock(queue->access_mutex);
    }
    pthread_mutex_unlock(queue->operation_mutex);

    Message* newMsg = (Message*)malloc(sizeof(Message)); 
    if (newMsg == NULL) {
        pthread_mutex_unlock(queue->access_mutex);
        //perror("Memory allocation failed!\n");
        return;
    }
    if (queue->subList->size == 0) {
        pthread_mutex_unlock(queue->access_mutex);
        //printf("No active subscribers - exiting function...\n");
        return;
    }
    newMsg->content = msg;
    newMsg->next = NULL;
    newMsg->readCount = queue->subList->size;
    newMsg->firstSub = queue->subList->head;

    if (queue->msgList->head == NULL) {
        queue->msgList->head = newMsg;
        queue->msgList->tail = newMsg;
    }
    else { 
        ((Message*)queue->msgList->tail)->next = newMsg;
        queue->msgList->tail = newMsg;
    }
    queue->msgList->size++;
    
    // updating the list of messages for all active subscribers
    Subscriber* currSub = queue->subList->head;
    while(currSub != NULL) {
        if (currSub->startReading == NULL) { 
            currSub->startReading = newMsg;
        }
        currSub = currSub->next;
    }
    // waking up the threads that are waiting for new messages
    pthread_mutex_lock(queue->operation_mutex);
    pthread_cond_broadcast(queue->block_operation); 
    pthread_mutex_unlock(queue->operation_mutex);

    pthread_mutex_unlock(queue->access_mutex);
    //printf("Message added\n");
}

void* getMsg(TQueue* queue, pthread_t* thread) { 

    pthread_mutex_lock(queue->access_mutex);

    Subscriber* temp = queue->subList->head; //temp is a current thread
    while (temp->threadID != thread) {
        temp = temp->next;
    }
    if (temp == NULL) {
        //printf("This thread is not subsrcibing this queue.\n");
        pthread_mutex_unlock(queue->access_mutex);
        return NULL;
    }
    // blocking behaviour when the list of messages is empty
    pthread_mutex_lock(queue->operation_mutex);
    while(temp->startReading == NULL) {
        //printf("The list of messages for this subscriber is empty. Waiting...\n"); 
        pthread_mutex_unlock(queue->access_mutex); 
        pthread_cond_wait(queue->block_operation, queue->operation_mutex);
        pthread_mutex_lock(queue->access_mutex); 
    }
    pthread_mutex_unlock(queue->operation_mutex); 
    Message* receivedMsg = temp->startReading;
    temp->startReading->readCount--;
    // checking if the received message might be deleted
    if (temp->startReading->readCount == 0) {
        delMsg(queue, temp->startReading);
    }
    // updating the list of messages for this thread
    temp->startReading = temp->startReading->next; 
    pthread_mutex_unlock(queue->access_mutex);

    return receivedMsg;
}

int getAvailable(TQueue* queue, pthread_t* thread) {

    int count = 0;

    pthread_mutex_lock(queue->access_mutex);
    Subscriber* tempSub = queue->subList->head;
    while (tempSub->threadID != thread) {
        tempSub = tempSub->next;
    }
    if (tempSub == NULL) {
        pthread_mutex_unlock(queue->access_mutex);
        //printf("This thread doesn't subscribe this queue.\n");
        return count;
    }
    else {
        Message* tempMsg = tempSub->startReading;
        // traversing the list and counting the elements
        while (tempMsg != NULL) {
            count++;
            tempMsg = tempMsg->next;
        }
    }
    pthread_mutex_unlock(queue->access_mutex);
    return count;
}

void removeMsg(TQueue* queue, void* msg) { 

    pthread_mutex_lock(queue->access_mutex);

    if (queue == NULL || queue->msgList->head == NULL) {
        pthread_mutex_unlock(queue->access_mutex);
        //printf("Element not found!\n");
        return;
    }

    if (((Message *)queue->msgList->head)->content == msg) { 
        queue->msgList->head = ((Message *)queue->msgList->head)->next; 
        queue->msgList->size--;
    }
    else { 
        Message* prevMsg = queue->msgList->head;
        while (prevMsg->next != NULL) {
            if (prevMsg->next->content == msg) {
                prevMsg->next = prevMsg->next->next;
                //printf("Element removed!\n");
                queue->msgList->size--;
                break;  
            }
            prevMsg = prevMsg->next;
        }
        if (prevMsg->next == NULL) { 
            //printf("Element not found!\n");
            pthread_mutex_unlock(queue->access_mutex);
            return;
        }
    }
    // checking for each subscriber if the removed message is the first message on its list of messages
    Subscriber* tempSub = queue->subList->head;
    while (tempSub != NULL) {
        if (tempSub->startReading->content == msg) {
            tempSub->startReading = tempSub->startReading->next; 
        }
        tempSub = tempSub->next;
    }

   //waking a thread that is waiting for freeing up space in the queue
    pthread_mutex_lock(queue->operation_mutex);
    pthread_cond_signal(queue->block_operation);  
    pthread_mutex_unlock(queue->operation_mutex);
    pthread_mutex_unlock(queue->access_mutex);
    //printf("Element removed successfully!\n");
    return;
}

void setSize(TQueue* queue, int* newSize) { 
    
    pthread_mutex_lock(queue->access_mutex);
    //printf("Setting new size...\n");
    int currSize = queue->msgList->size;
    if (*newSize < currSize) { 
        Subscriber* tempSub;
        //printf("New size is smaller than the current queue size\n");
        Message* curr = queue->msgList->head;
        // removing first n messages (calculated based on new size)
        for (int i = 0; i < currSize - *newSize; i++) {
            // removing messages from subscribers' lists of messages
            tempSub = queue->subList->head;
            while (tempSub != NULL) {
                if (tempSub->startReading->content == curr->content) {
                    tempSub->startReading = tempSub->startReading->next;
                }
                tempSub = tempSub->next;
            }

            curr = curr->next;
            queue->msgList->size--;
        }
        queue->msgList->head = curr;
    }
    // waking up the threads that are waiting for freeing up space in the queue
    pthread_mutex_lock(queue->operation_mutex);
    pthread_cond_broadcast(queue->block_operation);
    pthread_mutex_unlock(queue->operation_mutex);
    queue->maxSize = *newSize;
    //printf("New size has been set successfully.\n");
    pthread_mutex_unlock(queue->access_mutex);
}