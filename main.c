#include "queue.h"

void* thread_handler(void* arg) {

    char* msg = "one";
    char* msg2 = "two";
    char* msg3 = "three";
    //char* msg4 = "four";
    char* msg5 = "five";
    TQueue* queue = (TQueue*)arg;
    pthread_t threadID = pthread_self();
    pthread_t *ptrID = &threadID;
    sleep(1);
    addMsg(queue, msg);
    addMsg(queue, msg2);
    addMsg(queue, msg3);
    //addMsg(queue, msg4); 
    addMsg(queue, msg5);
    subscribe(queue, ptrID);
    printf("Available for thread 1: %d\n", getAvailable(queue, ptrID));

    return NULL;
}

void* thread2_handler(void* arg) {

    int size = 8;
    int* ptr = &size;
    //char* msg = "six";

    TQueue* queue = (TQueue*)arg;
    pthread_t threadID = pthread_self();
    pthread_t *ptrID = &threadID;

    subscribe(queue, ptrID);
    printf("Available for thread 2: %d\n", getAvailable(queue, ptrID));
    sleep(4);
    setSize(queue, ptr);
    printf("Available for thread 2: %d\n", getAvailable(queue, ptrID));

    return NULL;
}

void* thread3_handler(void* arg) {

    char* msg = "seven";
    char* msg2 = "eight";

    TQueue* queue = (TQueue*)arg;

    addMsg(queue, msg);
    addMsg(queue, msg2);
    removeMsg(queue, msg);


    return NULL;
}

int main() {
    int T = 3;
    pthread_t threads[T];

    int size = 3;
    int* ptr = &size;
    printf("Start\n");
    TQueue *queue = createQueue(ptr);

    if(pthread_create(&threads[0], NULL, thread_handler, (void*)queue) != 0){
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
    for (int i = 0; i < T; i++) {
        pthread_join(threads[i], NULL);
    }
    destroyQueue(&queue);
    return 0;
}