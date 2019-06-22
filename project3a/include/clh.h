#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    int locked;
} qnode_t;

typedef struct {
    volatile qnode_t *tail;
} clh_t;

qnode_t *qnode_init(int locked);

int clh_init(clh_t *clh, __attribute__((unused)) int unused);

// return pointer to my_pred
volatile qnode_t *clh_lock(clh_t *clh, volatile qnode_t *my_node, volatile qnode_t *my_pred);

// return pointer to my_pred if acquired the lock and NULL otherwise
volatile qnode_t *clh_trylock(clh_t *clh, volatile qnode_t *my_node, volatile qnode_t *my_pred);

// return pointer to my_node
volatile qnode_t *clh_unlock(__attribute__((unused)) clh_t *clh, volatile qnode_t *my_node, 
    volatile qnode_t *my_pred);

int clh_destroy(clh_t *clh);