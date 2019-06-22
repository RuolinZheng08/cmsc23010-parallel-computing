#include "driver.h"

extern StopWatch_t watch;
extern int last_index;
extern int timeout_flag;

int main(int argc, char **argv) {
    int opt;
    int unif_flag = 1;  // use uniform packets if 1, exp if 0
    int num_src = -1;
    long num_msec = -1;  // int is too small to cast to void *
    int depth = -1;
    int mean = -1;
    char *lock_type = NULL;
    char *strategy = NULL;
    int seed = 0;

    while ((opt = getopt(argc, argv, "n:m:d:w:u:l:s:e:")) != -1) {
        switch(opt) {
            case 'n':
                num_src = atoi(optarg);
                break;
            case 'm':
                num_msec = atol(optarg);
                break;
            case 'd':
                depth = atoi(optarg);
                break;
            case 'w':
                mean = atoi(optarg);
                break;
            case 'u':
                unif_flag = atoi(optarg);
                break;
            case 'l':
                lock_type = strdup(optarg);
                break;
            case 's':
                strategy = strdup(optarg);
                break;
            case 'e':
                seed = atoi(optarg);
                break;
        }
    }

    if (num_src == -1 || num_msec == -1 || mean == -1) {
        printf("Usage: \n./driver -m NUM_MILLISECONDS -n NUM_SOURCES -w MEAN "
        "-u [1|0] -d DEPTH -l [clh|mutex] -s [lockfree|homequeue|awesome] "
        "-e seed\n"
        "Serial example: ./driver -m 2000 -n 7 -w 1000 -u 1\n"
        "Parallel example: ./driver -m 2000 -n 7 -w 1000 -u 1 -d 8 "
        "-l clh -s homequeue\n");
        exit(1);
    }
    // need for fast modulo
    if (strategy != NULL && depth % 2 != 0) {
        printf("Please choose a number 2^x for depth. \n");
        exit(1);
    }

    // a pointer to the specified packet generation function
    volatile Packet_t *(*get_packet)(PacketSource_t *, int);
    if (unif_flag != 0) {
        get_packet = &getUniformPacket;
    } else {
        get_packet = &getExponentialPacket;
    }
    PacketSource_t *source = createPacketSource(mean, num_src, seed);

    // run serial or parallel, start timer thread after lock initialization
    int serial_flag = 1;
    if (depth != -1 && strategy != NULL) {
        if ((strcmp(strategy, "lockfree") == 0) || lock_type != NULL) {
            serial_flag = 0;
        } else {
            printf("Usage: \n./driver -m NUM_MILLISECONDS -n NUM_SOURCES -w MEAN "
            "-u [1|0] -d DEPTH -l [clh|mutex] -s [lockfree|homequeue|awesome] "
            "-e seed\n"
            "Serial example: ./driver -m 2000 -n 7 -w 1000 -u 1\n"
            "Parallel example: ./driver -m 2000 -n 7 -w 1000 -u 1 -d 8 "
            "-l clh -s homequeue\n");
            exit(1);
        }
    }

    printf("Program: m=%ld, n=%d, w=%d, u=%d, d=%d, e=%d, %s, %s\n", 
        num_msec, num_src, mean, unif_flag, depth, seed, strategy, lock_type);

    // core program
    long total_pkt;
    pthread_t timer;
    int rc;
    if (serial_flag) {

        rc = pthread_create(&timer, NULL, timer_func, (void *) num_msec);
        total_pkt = serial(source, get_packet, num_src, -1);  // use -1 for num_pkt
        pthread_join(timer, NULL);

    } else {
        shared_t *shared = init_shared(num_src, -1, depth, lock_type);
        void *(*worker_strategy)(void *) = NULL;

        if (strcmp(strategy, "lockfree") == 0) {
            worker_strategy = &worker_lockfree;
        } else if (strcmp(strategy, "homequeue") == 0) {
            if (strcmp(lock_type, "mutex") == 0) {
                worker_strategy = &worker_homequeue_mutex;
            } else if (strcmp(lock_type, "clh") == 0) {
                worker_strategy = &worker_homequeue_clh;
            }
        } else if (strcmp(strategy, "awesome") == 0) {
            if (strcmp(lock_type, "mutex") == 0) {
                worker_strategy = &worker_awesome_mutex;
            } else if (strcmp(lock_type, "clh") == 0) {
                worker_strategy = &worker_awesome_clh;
            }
        } else {
            printf("Invalid strategy: %s.\n", strategy);
            exit(1);
        }

        rc = pthread_create(&timer, NULL, timer_func, (void *) num_msec);
        total_pkt = parallel(source, get_packet, worker_strategy, shared);
        pthread_join(timer, NULL);
        destroy_shared(shared);
    }
    rc--;
    printf("Total Time Handling Packets: %lf\n", getElapsedTime(&watch));
    printf("Total Packets: %ld\n", total_pkt);

    // clean up
    deletePacketSource(source);

    return 0;
}

/* Begin Worker Functions */
void *worker_lockfree(void *args) {
    long counter = 0;  // int is too small to cast to void *
    long finger;
    int rc;
    volatile Packet_t *pkt = NULL;

    targs_t *targs = args;
    int tid = targs->tid;
    shared_t *shared = targs->shared;
    lamport_queue_t *my_queue = shared->all_queues[tid];

    while (timeout_flag == 0) {
        pkt = (Packet_t *) dequeue(my_queue);
        if (pkt != NULL) {
            finger = getFingerprint(pkt->iterations, pkt->seed);
            counter++;
        } else {
            rc = sched_yield();
        }
    }
    rc--;  // turn off compiler warning
    finger--;
    pthread_exit((void *) counter);
}

void *worker_homequeue_mutex(void *args) {
    long counter = 0;
    long finger;
    int rc;
    volatile Packet_t *pkt = NULL;

    targs_t *targs = args;
    int tid = targs->tid;
    shared_t *shared = targs->shared;
    lamport_queue_t *my_queue = shared->all_queues[tid];
    pthread_mutex_t *my_mutex = shared->all_locks[tid];

    while (timeout_flag == 0) {
        pthread_mutex_lock(my_mutex);
        pkt = (Packet_t *) dequeue(my_queue);
        pthread_mutex_unlock(my_mutex);
        if (pkt != NULL) {
            finger = getFingerprint(pkt->iterations, pkt->seed);
            counter++;
        } else {
            rc = sched_yield();
        }
    }
    rc--;  // turn off compiler warning
    finger--;
    pthread_exit((void *) counter);
}

void *worker_lastqueue_mutex(void *args) {
    long counter = 0;
    long finger;
    int rc;
    int idx;  // local copy of last_index
    volatile Packet_t *pkt = NULL;

    targs_t *targs = args;
    int tid = targs->tid;
    shared_t *shared = targs->shared;
    int num_src = shared->num_src;

    lamport_queue_t **queues = shared->all_queues;
    lamport_queue_t *my_queue = queues[tid];
    pthread_mutex_t **mutexes = (pthread_mutex_t **) shared->all_locks;
    pthread_mutex_t *my_mutex = mutexes[tid];

    while (timeout_flag == 0) {
        // first try the direct mapping queue, if empty, try the last one
        pthread_mutex_lock(my_mutex);
        pkt = (Packet_t *) dequeue(my_queue);
        pthread_mutex_unlock(my_mutex);
        if (pkt != NULL) {
            finger = getFingerprint(pkt->iterations, pkt->seed);
            counter++;
        } else {
            idx = last_index;
            rc = pthread_mutex_trylock(mutexes[idx]);
            if (rc == 0) {
                last_index = (idx + 1) & (num_src - 1);  // x % y == x & (y - 1)
                pkt = (Packet_t *) dequeue(queues[idx]);
                pthread_mutex_unlock(mutexes[idx]);
                if (pkt != NULL) {
                    // possible that some one dequeues the last item enqueued
                    // by the dispatcher before this thread acquires the lock
                    finger = getFingerprint(pkt->iterations, pkt->seed);
                    counter++;
                }
            }
            rc = sched_yield();
        }
    }
    rc--;  // turn off compiler warning
    finger--;
    pthread_exit((void *) counter);
}

void *worker_homequeue_clh(void *args) {
    long counter = 0;
    long finger;
    int rc;
    volatile Packet_t *pkt = NULL;

    targs_t *targs = args;
    int tid = targs->tid;
    shared_t *shared = targs->shared;

    lamport_queue_t *my_queue = shared->all_queues[tid];
    clh_t *my_clh = shared->all_locks[tid];
    volatile qnode_t *my_node = qnode_init(1);
    volatile qnode_t *my_pred = NULL;

    while (timeout_flag == 0) {
        my_pred = clh_lock(my_clh, my_node, my_pred);
        pkt = (Packet_t *) dequeue(my_queue);
        my_node = clh_unlock(my_clh, my_node, my_pred);
        if (pkt != NULL) {
            finger = getFingerprint(pkt->iterations, pkt->seed);
            counter++;
        } else {
            rc = sched_yield();
        }
    }
    rc--;  // turn off compiler warning
    finger--;
    pthread_exit((void *) counter);
}

void *worker_lastqueue_clh(void *args) {
    long counter = 0;
    long finger;
    int rc;
    int idx;  // local copy of last_index
    volatile Packet_t *pkt = NULL;

    targs_t *targs = args;
    int tid = targs->tid;
    shared_t *shared = targs->shared;
    int num_src = shared->num_src;

    lamport_queue_t **queues = shared->all_queues;
    lamport_queue_t *my_queue = queues[tid];
    clh_t **clhs = (clh_t **) shared->all_locks;
    clh_t *my_clh = clhs[tid];


    volatile qnode_t *my_node = qnode_init(1);
    volatile qnode_t *my_pred = NULL;
    volatile qnode_t *ret = NULL;

    while (timeout_flag == 0) {
        // first try the direct mapping queue, if empty, try the last one
        my_pred = clh_lock(my_clh, my_node, my_pred);
        pkt = (Packet_t *) dequeue(my_queue);
        my_node = clh_unlock(my_clh, my_node, my_pred);
        if (pkt != NULL) {
            finger = getFingerprint(pkt->iterations, pkt->seed);
            counter++;
        } else {
            idx = last_index;
            ret = clh_trylock(clhs[idx], my_node, my_pred);  // return NULL if lock is busy
            if (ret != NULL) {
                last_index = (idx + 1) & (num_src - 1);
                my_pred = ret;  // my pred has unlocked
                pkt = (Packet_t *) dequeue(queues[idx]);
                my_node = clh_unlock(clhs[idx], my_node, my_pred);
                if (pkt != NULL) {
                    // possible that some one dequeues the last item enqueued
                    // by the dispatcher before this thread acquires the lock
                    finger = getFingerprint(pkt->iterations, pkt->seed);
                    counter++;
                }
            }
            rc = sched_yield();
        }
    }
    rc--;  // turn off compiler warning
    finger--;
    pthread_exit((void *) counter);
}
/* End Worker Functions */