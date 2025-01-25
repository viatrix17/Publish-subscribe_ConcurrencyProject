#include "queue.h"

void* thread_handler(void* arg) {

    int size = 6;
    int* ptr = &size;
    char* msg = "first";
    char* msg2 = "second";
    char* msg3 = "GLORIOUS EVOLUTION";
    TQueue* queue = (TQueue*)arg;
    pthread_t threadID = pthread_self();
    pthread_t *ptrID = &threadID;
    subscribe(queue, ptrID);
    printf("sub size %d\n\n", queue->subList->size);

    addMsg(queue, msg);
    addMsg(queue, msg2);
    //addMsg(queue, msg3);
    printf("msg size %d\n\n", queue->msgList->size);
    printf("first message: %s\n", (char*)((Message*)queue->msgList->head)->content);
    printf("last message: %s\n", (char*)((Message*)queue->msgList->tail)->content);
    printf("sub first message: %s\n", (char*)((Subscriber*)queue->subList->head)->startReading->content);


    // printf("queue size %d\n", queue->maxSize);
    // setSize(queue, ptr);
    // printf("queue size %d\n", queue->maxSize);
    // printf("msg size %d\n", queue->msgList->size);
    // printf("first message: %s\n", (char*)((Message*)queue->msgList->head)->content);
    // printf("last message: %s\n", (char*)((Message*)queue->msgList->tail)->content);
    // printf("sub first message: %s\n", (char*)((Subscriber*)queue->subList->head)->startReading->content);
    // unsubscribe(queue, ptrID);
    // printf("size %d\n", queue->subList->size);
    // void* received = getMsg(queue, ptrID);
    // printf("msg: %s\n", (char*)((Message*)received)->content);
    // printf("size %d\n", queue->msgList->size);
    //printf("sub first message: %s\n", (char*)((Subscriber*)queue->subList->head)->startReading->content);
    //removeMsg(queue, msg);
    // printf("size %d\n", queue->msgList->size);
    // printf("fiest message: %s\n", (char*)((Message*)queue->msgList->head)->content);
    // printf("last message: %s\n", (char*)((Message*)queue->msgList->tail)->content);
    // printf("get available %d\n", getAvailable(queue, ptrID));
    // printf("sub first message: %s\n", (char*)((Subscriber*)queue->subList->head)->startReading->content);
    // void* received = getMsg(queue, ptrID);
    // printf("msg: %s\n", (char*)((Message*)received)->content);
    //received = getMsg(queue, ptrID);
    //printf("%s\n", (char*)((Message*)queue->msgList->head)->content);
    return NULL;
}

int main() {
    int T = 1;
    pthread_t threads[T];

    int size = 2;
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
    destroyQueue(&queue);
    return 0;
}