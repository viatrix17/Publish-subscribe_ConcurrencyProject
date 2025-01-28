#include "queue.h"

void* publish(void* arg) {

    char* msg = "one";
    char* msg2 = "two";
    char* msg3 = "three";
    char* msg4 = "four";
    char* msg5 = "five";
    

    TQueue* queue = (TQueue*)arg;

    //TEST 3
    sleep(1);
    addMsg(queue, msg);
    addMsg(queue, msg2);
    addMsg(queue, msg3);
    addMsg(queue, msg4);
    sleep(3);
    addMsg(queue, msg5);

    //TEST 2
    //
    // sleep(1);
    // addMsg(queue, msg);
    // sleep(2);
    // addMsg(queue, msg2);
    // sleep(3);
    // addMsg(queue, msg3);
    // sleep(5);
    // removeMsg(queue, msg2);
    // sleep(2);
    // addMsg(queue, msg4);
    // sleep(3);
    // setSize(queue, 2);
    
    
    //TEST 1
    //
    // sleep(3);
    // addMsg(queue, msg);
    // sleep(5);
    // addMsg(queue, msg2);
    

    return NULL;
}

void* thread2_handler(void* arg) {

    TQueue* queue = (TQueue*)arg;
    pthread_t threadID = pthread_self();  

    //TEST3
    //
    subscribe(queue, threadID);
    sleep(3);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    getMsg(queue, threadID);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    sleep(4);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    getMsg(queue, threadID);

    //TEST2
    //
    // subscribe(queue, threadID);
    // sleep(7);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    // sleep(5);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    // sleep(5);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));

    
    //sleep(1);
    
    // TEST 1
    //
    // sleep(1);
    // subscribe(queue, threadID);
    // sleep(2);
    // printf("Available for thread 1: %d\n", getAvailable(queue, threadID));
    // sleep(2);
    // printf("Available for thread 1: %d\n", getAvailable(queue, threadID));
    // sleep(6);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    // getMsg(queue, threadID);

    return NULL;
}

void* thread3_handler(void* arg) {

    TQueue* queue = (TQueue*)arg;
    pthread_t threadID = pthread_self();
    
    //TEST3
    //
    sleep(5);
    subscribe(queue, threadID);
    getMsg(queue, threadID);

    //TEST 2
    // sleep(2);
    // subscribe(queue, threadID);
    // sleep(5);
    // printf("Available for thread 2: %d\n\n", getAvailable(queue, threadID));
    // sleep(5);
    // printf("Available for thread 2: %d\n\n", getAvailable(queue, threadID));
    // sleep(5);
    // printf("Available for thread 2: %d\n\n", getAvailable(queue, threadID));


    //TEST 1
    // subscribe(queue, threadID);
    // sleep(2);
    // unsubscribe(queue, threadID);
    // sleep(2);
    // subscribe(queue, threadID);
    // printf("Available for thread 2: %d\n\n", getAvailable(queue, threadID));
    // sleep(6);
    // printf("Available for thread 2: %d\n\n", getAvailable(queue, threadID));


    return NULL;
}

void* thread4_handler(void* arg) {

    // TQueue* queue = (TQueue*)arg;
    // pthread_t threadID = pthread_self();
    
    //TEST 2
    //
    // sleep(5);
    // subscribe(queue, threadID);
    // sleep(2);
    // printf("Available for thread 3: %d\n\n", getAvailable(queue, threadID));
    // sleep(5);
    // printf("Available for thread 3: %d\n\n", getAvailable(queue, threadID));
    // sleep(2);
    // getMsg(queue, threadID);
    // sleep(3);
    // printf("Available for thread 3: %d\n\n", getAvailable(queue, threadID));

    return NULL;
}

int main() {
    int T = 4;
    pthread_t threads[T];

    int size = 3;

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
    if(pthread_create(&threads[3], NULL, thread4_handler, (void*)queue) != 0){
        perror("pthread_create failed\n");
        return -1;
    }

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);
    pthread_join(threads[3], NULL);


    destroyQueue(queue);
    return 0;
}