#include "./queue.h"
#include "Tests.h"


int verbose;


void* publish(void* arg) {

    char* msg = "one";
    char* msg2 = "two";
    char* msg3 = "three";
    char* msg4 = "four";

    TQueue* queue = (TQueue*)arg;
    // pthread_t threadID = pthread_self();

    //TEST 7
    //



    //TEST 6
    //
    // sleep(1);
    // addMsg(queue, msg);
    // addMsg(queue, msg2);
    // addMsg(queue, msg3);
    // addMsg(queue, msg4);

    //TEST 5
    //
    // subscribe(queue, threadID);
    // addMsg(queue, msg);
    // sleep(3);
    // addMsg(queue, msg);
    // removeMsg(queue, msg);
    // removeMsg(queue, msg2);
    // addMsg(queue, msg);
    // removeMsg(queue, msg);
    // removeMsg(queue, msg);

    //TEST 4
    //
    // subscribe(queue, threadID);
    // addMsg(queue, msg);
    // addMsg(queue, msg2);
    // addMsg(queue, msg3);
    // removeMsg(queue, msg);
    // // int y = getAvailable(queue, threadID);
    // // printf("%d\n", y);
    // removeMsg(queue, msg3);
    // // y = getAvailable(queue, threadID);
    // // printf("%d\n", y);
    // removeMsg(queue, msg3);
    // printf("ok\n");
    // y = getAvailable(queue, threadID);
    // printf("%d\n", y);


    // char* x = getMsg(queue, threadID);
    // printf("%s\n", x);
    // x = getMsg(queue, threadID);
    // printf("%s\n", x);

    //TEST 3
    //
    // sleep(1);
    // subscribe(queue, threadID);
    // addMsg(queue, msg);
    // addMsg(queue, msg2);
    // addMsg(queue, msg3);
   
    // addMsg(queue, msg4);
    // sleep(3);
    // addMsg(queue, msg5);

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
    char* msg = "one";

    // TEST 6
    
    sleep(1);
    subscribe(queue, threadID);
    sleep(4);
    unsubscribe(queue, threadID);

    //TEST 4
    // 
    // sleep(1);
    // subscribe(queue, threadID);
    // sleep(4);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));



    //TEST 3
    //
    // subscribe(queue, threadID);
    // sleep(3);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    // getMsg(queue, threadID);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    // sleep(4);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    // getMsg(queue, threadID);

    //TEST2
    //
    // subscribe(queue, threadID);
    // sleep(7);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    // sleep(5);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    // sleep(5);
    // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));

        
    //TEST 1
    //
    // sleep(1);
    // subscribe(queue, threadID);
    // sleep(2);
    // // printf("Available for thread 1: %d\n", getAvailable(queue, threadID));
    // sleep(2);
    // // printf("Available for thread 1: %d\n", getAvailable(queue, threadID));
    // sleep(6);
    // // printf("Available for thread 1: %d\n\n", getAvailable(queue, threadID));
    // // char* ok = getMsg(queue, threadID);
    // printf("%s\n", ok);

    return NULL;
}

void* thread3_handler(void* arg) {

    TQueue* queue = (TQueue*)arg;
    pthread_t threadID = pthread_self();
    
    //subscribe(queue, threadID);
    //TEST3
    //
    // sleep(5);
    // subscribe(queue, threadID);
    // getMsg(queue, threadID);

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
    //
    // subscribe(queue, threadID);
    // sleep(2);
    // unsubscribe(queue, threadID);
    // sleep(2);
    // subscribe(queue, threadID);
    // // printf("Available for thread 2: %d\n\n", getAvailable(queue, threadID));
    // sleep(6);
    // // printf("Available for thread 2: %d\n\n", getAvailable(queue, threadID));


    return NULL;
}

void* thread4_handler(void* arg) {

    TQueue* queue = (TQueue*)arg;
    pthread_t threadID = pthread_self();
    
    //TEST 8
    //
    for(int i = 0; i < 10; i++) {
        subscribe(queue, threadID);
        subscribe(queue, threadID);
        printf("%lu\n", (unsigned long)((Subscriber*)queue->subList->head)->threadID);
        unsubscribe(queue, threadID);
        unsubscribe(queue, threadID);
    }
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

void myTests() {

     int T = 4;
    pthread_t threads[T];

    int size = 3;

    TQueue *queue = createQueue(size);

    if(pthread_create(&threads[0], NULL, publish, (void*)queue) != 0){
        perror("pthread_create failed\n");
        return;
    }
    if(pthread_create(&threads[1], NULL, thread2_handler, (void*)queue) != 0){
        perror("pthread_create failed\n");
        return;
    }
    if(pthread_create(&threads[2], NULL, thread3_handler, (void*)queue) != 0){
        perror("pthread_create failed\n");
        return;
    }
    if(pthread_create(&threads[3], NULL, thread4_handler, (void*)queue) != 0){
        perror("pthread_create failed\n");
        return;
    }

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);
    pthread_join(threads[3], NULL);


    destroyQueue(queue);


}

int main() {

    // verifyTQueueWorksUnderStress();
    // printf("\n");
    // verifyRemovingMessage2();
    // verifyRemovingMessage3();
    // verifyRemovingMessage();
     //verifyAddingMessage();

    myTests();

    return 0;
}