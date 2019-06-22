#include "common.h"

StopWatch_t watch;
int timeout_flag = 0;  // 1 if time is up and program should exit
// an index for the queue the dispatcher has just enqueued into as well as its lock
int last_index = 0;

long serial(PacketSource_t *source, volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt) {
    long global_ret = 0;
    long finger;
    volatile Packet_t *pkt;

    // loop until the timeout_flag is 1 in driver.c
    startTimer(&watch);
    if (num_pkt == -1) {
        while (timeout_flag == 0) {
            for (int i = 0; i < num_src; i++) {
                pkt = (*get_packet)(source, i);
                finger = getFingerprint(pkt->iterations, pkt->seed);
                global_ret++;
            }
        }
    } else {
        for (int t = 0; t < num_pkt; t++) {
            for (int i = 0; i < num_src; i++) {
                pkt = (*get_packet)(source, i);
                finger = getFingerprint(pkt->iterations, pkt->seed);
                global_ret += finger;
            }
        }
    }
    stopTimer(&watch);
    return global_ret;
}

/* Begin Parallel */
shared_t *init_shared(int num_src, int num_pkt, int depth, char *lock_type) {
    shared_t *shared = calloc(1, sizeof(shared_t));
    // create the queues
    lamport_queue_t **queues = calloc(num_src, sizeof(lamport_queue_t *));
    for (int i = 0; i < num_src; i++) {
        queues[i] = init_queue(depth);
    }
    // create the locks
    void **locks = NULL;  // lockfree default
    if (lock_type != NULL) {
        locks = calloc(num_src, sizeof(void *));
        if (strcmp(lock_type, "mutex") == 0) {
            for (int i = 0; i < num_src; i++) {
                locks[i] = calloc(1, sizeof(pthread_mutex_t));
                pthread_mutex_init(locks[i], 0);
            }
        } else if (strcmp(lock_type, "clh") == 0) {
            for (int i = 0; i < num_src; i++) {
                locks[i] = calloc(1, sizeof(clh_t));
                clh_init(locks[i], 0);
            }
        } else {
            printf("Invalid lock type: %s.\n", lock_type);
            exit(1);
        }
    }
    shared->num_src = num_src;
    shared->num_pkt = num_pkt;
    shared->all_queues = queues;
    shared->all_locks = locks;
    return shared;
}

void destroy_shared(shared_t *shared) {
    for (int i = 0; i < shared->num_src; i++) {
        free(shared->all_queues[i]);
    }
    free(shared->all_queues);
    if (shared->all_locks != NULL) {
        for (int i = 0; i < shared->num_src; i++) {
            free(shared->all_locks[i]);
        }
        free(shared->all_locks);
    }  
    free(shared);
}

void *timer_func(void *args) {
    long num_msec = (long) args;
    struct timespec ts = {num_msec / 1000, num_msec % 1000 * 1000000L};
    nanosleep(&ts, NULL);
    timeout_flag = 1;
    pthread_exit(0);
}

long parallel(PacketSource_t *source,
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    void *(*worker_strategy)(void *), shared_t *shared) {

    int rc;
    volatile Packet_t *pkt;

    int num_src = shared->num_src;
    int num_pkt = shared->num_pkt;  // -1 in driver.c
    lamport_queue_t **all_queues = shared->all_queues;

    // create threads with the specified worker strategy
    pthread_t threads[num_src];
    targs_t targs_arr[num_src];
    for (int i = 0; i < num_src; i++) {
        targs_arr[i].tid = i;
        targs_arr[i].shared = shared;
        rc = pthread_create(&threads[i], NULL, worker_strategy, (void *)&targs_arr[i]);
        if (rc != 0) {
            printf("Error creating thread.\n");
            exit(1);
        }
    }

    // loop until the timeout_flag is 1 in driver.c
    startTimer(&watch);
    if (num_pkt == -1) {
        while (timeout_flag == 0) {
            for (int i = 0; i < num_src; i++) {
                pkt = (*get_packet)(source, i);
                while (enqueue(all_queues[i], (void *) pkt)) {
                    last_index = i;
                    // spin if queue is full, break if time is up
                    if (timeout_flag) {
                        break;
                    }
                }
                last_index = i;
            }
        }
        rc--;  // turn off compiler warning
    } else {
        for (int t = 0; t < num_pkt; t++) {
            for (int i = 0; i < num_src; i++) {
                pkt = (*get_packet)(source, i);
                while (enqueue(all_queues[i], (void *) pkt)) {
                    last_index = i;
                    // spin if queue is full
                }
                last_index = i;
            }
        }
        timeout_flag = 1;
    }
    
    // join threads and record total number of packets processed in driver.c
    long local_ret;  // int too small to cast to void *
    long global_ret = 0;
    for (int i = 0; i < num_src; i++) {
        pthread_join(threads[i], (void *) &local_ret);
        global_ret += local_ret;
    }

    stopTimer(&watch);

    return global_ret;
}