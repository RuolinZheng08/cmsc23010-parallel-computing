#include <stdlib.h>

typedef struct {
    volatile long head;
    volatile long tail;
    int depth;
    void **items;
} lamport_queue_t;

lamport_queue_t *init_queue(int depth);

// return 1 on failure and 0 on success
int enqueue(lamport_queue_t *queue, void *item);

// return NULL on failure and the first item on success
void *dequeue(lamport_queue_t *queue);