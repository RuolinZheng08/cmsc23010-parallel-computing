#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <criterion/criterion.h>
#include "lamport_queue.h"

Test(lamport_queue, init) {
    int depth = 32;
    lamport_queue_t *q = init_queue(depth);
    cr_expect(q->head == 0);
    cr_expect(q->tail == 0);
    cr_expect(q->depth == depth);
    for (int i = 0; i < depth; i++) {
        cr_expect(q->items[i] == NULL);
    }
}

Test(lamport_queue, enq_err) {
    lamport_queue_t *q = init_queue(1);
    int rc = enqueue(q, "a");
    cr_expect(rc == 0);
    rc = enqueue(q, "b");
    cr_expect(rc == 1);
    cr_expect(strcmp(q->items[q->head], "a") == 0);
    rc = enqueue(q, "c");
    cr_expect(rc == 1);
    cr_expect(strcmp(q->items[q->head], "a") == 0);
    cr_expect(q->head == 0 && q->tail == 1);
}

Test(lamport_queue, deq_err) {
    lamport_queue_t *q = init_queue(1);
    void *ret = dequeue(q);
    cr_expect(ret == NULL);
}

Test(lamport_queue, serial) {
    lamport_queue_t *q = init_queue(4);
    int rc;
    void *ret;

    // enq(a), deq(), enq(b), enq(c), enq(d)
    // enq(e), deq(), deq(), deq(), deq()
    // expect: 0, a, 0, 0, 0, 0, b, c, d, e
    rc = enqueue(q, "a");
    cr_expect(rc == 0);
    cr_expect(strcmp(q->items[q->head], "a") == 0);
    cr_expect(q->head == 0 && q->tail == 1);

    ret = dequeue(q);
    cr_expect(strcmp((char *)ret, "a") == 0);
    cr_expect(q->items[q->head] == NULL);
    cr_expect(q->head == 1 && q->tail == 1);

    rc = enqueue(q, "b");
    cr_expect(rc == 0);
    cr_expect(strcmp(q->items[q->head], "b") == 0);
    cr_expect(q->head == 1 && q->tail == 2);

    rc = enqueue(q, "c");
    cr_expect(rc == 0);
    rc = enqueue(q, "d");
    cr_expect(rc == 0);
    rc = enqueue(q, "e");
    cr_expect(rc == 0);

    ret = dequeue(q);
    cr_expect(strcmp((char *)ret, "b") == 0);
    ret = dequeue(q);
    cr_expect(strcmp((char *)ret, "c") == 0);
    ret = dequeue(q);
    cr_expect(strcmp((char *)ret, "d") == 0);
    ret = dequeue(q);
    cr_expect(strcmp((char *)ret, "e") == 0);
}

// parallel tests
void test_two_threads(void *(*func_0)(void *), void *(*func_1)(void *),
    int time) {
    lamport_queue_t *q = init_queue(4);
    int rc = 0;
    rc++;  // turn off compiler warning
    pthread_t threads[2];

    rc = pthread_create(&threads[0], NULL, func_0, (void *) q);
    sleep(time);  // manually delaying the second thread
    rc = pthread_create(&threads[1], NULL, func_1, (void *) q);
    
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
}

// parallel sleep
// t0: enq(x) - sleep - sleep - deq(y)
// t1: sleep - enq(y) - deq(x)
void *worker_0_sleep(void *targs) {
    int rc;
    void *ret;
    lamport_queue_t *q = (lamport_queue_t *) targs;
    rc = enqueue(q, "x");
    cr_expect(rc == 0);
    sleep(2);
    ret = dequeue(q);
    cr_expect(strcmp(ret, "y") == 0);
    return NULL;
}

void *worker_1_sleep(void *targs) {
    int rc;
    void *ret;
    lamport_queue_t *q = (lamport_queue_t *) targs;
    sleep(1);
    rc = enqueue(q, "y");
    cr_expect(rc == 0);
    ret = dequeue(q);
    cr_expect(strcmp(ret, "x") == 0);
    return NULL;
}

Test(lamport_queue, parallel_sleep) {
    test_two_threads(worker_0_sleep, worker_1_sleep, 0);
}

// parallel spin
// dispatcher: spin till successfully enq(x)
// worker: spin till deq() returns non-NULL
void *dispatcher_spin(void *targs) {
    int rc;
    lamport_queue_t *q = (lamport_queue_t *) targs;
    char *elms[10] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
    int counter = 0;
    while (counter < 10) {
        rc = enqueue(q, elms[counter]);
        if (rc == 0) {
            counter++;
        }
    }
    return NULL;
}

void *worker_spin(void *targs) {
    void *ret = NULL;
    lamport_queue_t *q = (lamport_queue_t *) targs;
    char *elms[10] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
    int counter = 0;
    while (counter < 10) {
        ret = dequeue(q);
        if (ret != NULL) {
            cr_expect(strcmp((char *)ret, elms[counter]) == 0,
                "Expect: %s, got: %s", elms[counter], (char *)ret);
            counter++;
        }
    }
    return NULL;
}

Test(lamport_queue, parallel_spin) {
    test_two_threads(dispatcher_spin, worker_spin, 0);
}

Test(lamport_queue, parallel_spread_out) {
    test_two_threads(dispatcher_spin, worker_spin, 3);
}