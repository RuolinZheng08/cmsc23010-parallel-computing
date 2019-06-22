#include "clh.h"

qnode_t *qnode_init(int locked) {
    qnode_t *qnode = calloc(1, sizeof(qnode_t));
    qnode->locked = locked;
    return qnode;
}

int clh_init(clh_t *clh, __attribute__((unused)) int unused) {
    clh->tail = qnode_init(0);
    return 0;
}

volatile qnode_t *clh_lock(clh_t *clh, volatile qnode_t *my_node, 
    volatile qnode_t *my_pred) {
    my_node->locked = 1;
    my_pred = __sync_lock_test_and_set(&(clh->tail), my_node);
    while (my_pred->locked) {
        // spin
    }
    return my_pred;
}

volatile qnode_t *clh_trylock(clh_t *clh, volatile qnode_t *my_node, 
    volatile qnode_t *my_pred) {
    if (clh->tail->locked) {
        return NULL;
    } else {
        my_node->locked = 1;
        my_pred = __sync_lock_test_and_set(&(clh->tail), my_node);
        while (my_pred->locked) {
            // spin
        }
        return my_pred;
    }
}

volatile qnode_t *clh_unlock(__attribute__((unused)) clh_t *clh, 
    volatile qnode_t *my_node, volatile qnode_t *my_pred) {
    my_node->locked = 0;
    return my_pred;
}

int clh_destroy(clh_t *clh) {
    free((void *) clh->tail);
    free(clh);
    return 0;
}