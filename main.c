#include "queue.h"

void* thread_handler(void* arg) {
    int i;
    char* msg = "oki";
    TQueue* queue = (TQueue*)arg;
    pthread_t threadID = pthread_self();
    pthread_t *ptrID = &threadID;
    subscribe(queue, ptrID);
    printf("size %d\n", queue->subList->size);
    // subscribe(queue, ptrID);
    // printf("size %d\n", queue->subList->size);
    addMsg(queue, msg);
    printf("size %d\n", queue->msgList->size);
    printf("first message: %s\n", (char*)((Subscriber*)queue->subList->head)->startReading->content);
    // unsubscribe(queue, ptrID);
    // printf("size %d\n", queue->subList->size);
    addMsg(queue, msg);
    printf("size %d\n", queue->msgList->size);
    printf("get avail %d\n", getAvailable(queue, ptrID));
    void* received = getMsg(queue, ptrID);
    printf("msg: %s\n", (char*)((Message*)received)->content);
    printf("size %d\n", queue->msgList->size);
    printf("get available %d\n", getAvailable(queue, ptrID));
    //printf("%s\n", (char*)((Message*)queue->msgList->head)->content);
    //retyrb stg
}

int main() {
    int T = 1;
    pthread_t threads[T];

    int size = 5;
    int* ptr = &size;
    printf("Start\n");
    TQueue *queue = createQueue(ptr);

    for (int i = 0; i < T; i++) {
        if(pthread_create(&threads[i], NULL, thread_handler, (void*)queue) != 0){
            perror("pthread_create failed\n");
            return -1;
        }
    }
    for (int i = 0; i < T; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}