#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <criterion/criterion.h>
#include "clh.h"

typedef struct {
    int *counter;
    clh_t *clh;
} clh_targs_t;

Test(clh, init) {
    clh_t *clh = calloc(1, sizeof(clh_t));
    clh_init(clh, 0);
    cr_expect(clh->tail->locked == 0);
}

Test(clh, lock_and_unlock) {
    clh_t *clh = calloc(1, sizeof(clh_t));
    clh_init(clh, 0);
    
    volatile qnode_t *my_node = qnode_init(1);
    volatile qnode_t *my_pred = NULL;

    qnode_t *old_tail = (qnode_t *) clh->tail;
    my_pred = clh_lock(clh, my_node, my_pred);
    cr_expect(clh->tail->locked == 1);
    cr_expect(my_node == clh->tail);
    cr_expect(my_pred == old_tail);
    qnode_t *old_node = (qnode_t *) my_node;

    my_node = clh_unlock(clh, my_node, my_pred);
    cr_expect(clh->tail->locked == 0);
    cr_expect(my_node != old_node);
    cr_expect((my_node == my_pred) && (my_pred == old_tail));
    cr_expect(old_node == clh->tail);

    old_tail = (qnode_t *) clh->tail;
    my_pred = clh_lock(clh, my_node, my_pred);
    cr_expect(clh->tail->locked == 1);
    cr_expect(my_node == clh->tail);
    cr_expect(my_pred == old_tail);
    old_node = (qnode_t *) my_node;

    my_node = clh_unlock(clh, my_node, my_pred);
    cr_expect(clh->tail->locked == 0);
    cr_expect(my_node != old_node);
    cr_expect((my_node == my_pred) && (my_pred == old_tail));
    cr_expect(old_node == clh->tail);
}

// parallel
// return the resultant counter value
int test_two_threads(void *(*func_0)(void *), void *(*func_1)(void *)) {
    int rc = 0;
    rc++;  // turn off compiler warning
    pthread_t threads[2];
    clh_t *clh = calloc(1, sizeof(clh_t));
    clh_init(clh, 0);
    int *counter = calloc(1, sizeof(counter));
    *counter = 1;
    clh_targs_t targs;

    targs.counter = counter;
    targs.clh = clh;
    rc = pthread_create(&threads[0], 0, func_0, (void *) &targs);
    rc = pthread_create(&threads[1], 0, func_1, (void *) &targs);
    pthread_join(threads[0], 0);
    pthread_join(threads[1], 0);
    return *counter;
}

// spread out in time
// t0: first adds 10 to the variable and sleep
// t1: then multiplies it by 3 and exit
// t0: wakes up and adds 10, result should be 43
void *worker_0_sleep(void *args) {
    clh_targs_t *targs = args;
    int *counter = targs->counter;
    clh_t *clh = targs->clh;
    volatile qnode_t *my_node = qnode_init(1);
    volatile qnode_t *my_pred = NULL;
    my_pred = clh_lock(clh, my_node, my_pred);
    *counter += 10;
    my_node = clh_unlock(clh, my_node, my_pred);
    sleep(2);
    my_pred = clh_lock(clh, my_node, my_pred);
    *counter += 10;
    my_node = clh_unlock(clh, my_node, my_pred);
    pthread_exit(0);
}

void *worker_1_sleep(void *args) {
    clh_targs_t *targs = args;
    int *counter = targs->counter;
    clh_t *clh = targs->clh;
    volatile qnode_t *my_node = qnode_init(1);
    volatile qnode_t *my_pred = NULL;
    sleep(1);
    my_pred = clh_lock(clh, my_node, my_pred);
    *counter *= 3;
    my_node = clh_unlock(clh, my_node, my_pred);
    pthread_exit(0);
}

Test(clh, sleep_lock_and_unlock) {
    int ret;
    for (int i = 0; i < 5; i++) {
        printf("CLH: Running sleep trial %d\n", i);
        ret = test_two_threads(worker_0_sleep, worker_1_sleep);
        cr_expect(ret == 43, "Result: %d\n", ret);
    }
}

// race
// t0: increments the counter from 0 to 100000000 while holding lock
// t0: takes about 200ms
// t1: sleeps for 100ms, tries to acquire the lock but keeps spinning
// t1: eventually acquires the lock and negate the value
void *worker_0_race(void *args) {
    clh_targs_t *targs = args;
    int *counter = targs->counter;
    clh_t *clh = targs->clh;
    volatile qnode_t *my_node = qnode_init(1);
    volatile qnode_t *my_pred = NULL;
    my_pred = clh_lock(clh, my_node, my_pred);
    while (*counter < 100000000) {
        *counter += 1;
    }
    my_node = clh_unlock(clh, my_node, my_pred);
    sleep(1);
    cr_expect(clh->tail->locked == false);  // t1 has finished
    my_pred = clh_lock(clh, my_node, my_pred);  // t0 can still lock and unlock
    my_node = clh_unlock(clh, my_node, my_pred);
    pthread_exit(0);
}

void *worker_1_race(void *args) {
    clh_targs_t *targs = args;
    int *counter = targs->counter;
    clh_t *clh = targs->clh;
    volatile qnode_t *my_node = qnode_init(1);
    volatile qnode_t *my_pred = NULL;
    usleep(10000);
    my_pred = clh_lock(clh, my_node, my_pred);
    *counter = -(*counter);
    my_node = clh_unlock(clh, my_node, my_pred);
    pthread_exit(0);
}

Test(clh, race_lock_and_unlock) {
    int ret;
    for (int i = 0; i < 5; i++) {
        printf("CLH: Running race trial %d\n", i);
        ret = test_two_threads(worker_0_race, worker_1_race);
        cr_expect(ret == -100000000, "Result: %d\n", ret);
    }
}