#include "lamport_queue.h"

lamport_queue_t *init_queue(int depth) {
    lamport_queue_t *queue = (lamport_queue_t *)calloc(1, sizeof(lamport_queue_t));
    queue->head = 0;
    queue->tail = 0;
    queue->depth = depth;
    queue->items = (void **) calloc(depth, sizeof(void*));
    return queue;
}

int enqueue(lamport_queue_t *queue, void *item) {
    if (queue->tail - queue->head == queue->depth) {
        return 1;  // full
    }
    queue->items[queue->tail % queue->depth] = item;
    __sync_synchronize();
    queue->tail++;
    return 0;
}

void *dequeue(lamport_queue_t *queue) {
    if (queue->head == queue->tail) {
        return NULL;  // empty
    }
    void *item = queue->items[queue->head % queue->depth];
    __sync_synchronize();
    queue->head++;
    return item;
}