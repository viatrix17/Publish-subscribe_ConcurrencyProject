#include "include.h"

pthread_mutex_t operations_mutex;

int main() {
    pthread_mutex_init(&operations_mutex, NULL);
    pthread_mutex_destroy(&operations_mutex);
    return 0;
}