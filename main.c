#include "queue.h"

void* publish(void* arg) {

    char* msg = "one";
    char* msg2 = "two";
    // char* msg3 = "three";
    // char* msg4 = "four";
    // char* msg5 = "five";
    TQueue* queue = (TQueue*)arg;
    sleep(3);
    addMsg(queue, msg);
    sleep(5);
    addMsg(queue, msg2);
    // addMsg(queue, msg3);
    // addMsg(queue, msg4); 
    // addMsg(queue, msg5);

    return NULL;
}

void* thread2_handler(void* arg) {

    TQueue* queue = (TQueue*)arg;
    pthread_t threadID = pthread_self();   

    sleep(1);
    subscribe(queue, threadID);
    sleep(2);
    printf("Available for thread 1: %d\n", getAvailable(queue, threadID));
    sleep(2);
    printf("Available for thread 1: %d\n", getAvailable(queue, threadID));
    sleep(6);
    printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    char* receivedMsg = getMsg(queue, threadID);
    if (receivedMsg != NULL) {
        printf("message received by thread 1: %s\n", (char*)receivedMsg);
    }

    return NULL;
}

void* thread3_handler(void* arg) {

    TQueue* queue = (TQueue*)arg;
    pthread_t threadID = pthread_self();
    
    subscribe(queue, threadID);
    sleep(2);
    unsubscribe(queue, threadID);
    sleep(2);
    subscribe(queue, threadID);
    printf("Available for thread 2: %d\n\n", getAvailable(queue, threadID));
    sleep(6);
    printf("Available for thread 2: %d\n\n", getAvailable(queue, threadID));


    return NULL;
}

int main() {
    int T = 3;
    pthread_t threads[T];

    int size = 2;

    TQueue *queue = createQueue(size);

    if(pthread_create(&threads[0], NULL, publish, (void*)queue) != 0){
        perror("pthread_create failed\n");
        return -1;
    }
    if(pthread_create(&threads[1], NULL, thread2_handler, (void*)queue) != 0){
        perror("pthread_create failed\n");
        return -1;
    }
    if(pthread_create(&threads[2], NULL, thread3_handler, (void*)queue) != 0){
        perror("pthread_create failed\n");
        return -1;
    }
   
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);

    destroyQueue(queue);
    return 0;
}