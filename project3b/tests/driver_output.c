#include "driver_output.h"

// from common.c
extern StopWatch_t watch;
extern int last_index;
extern int timeout_flag;

int main(int argc, char **argv) {
    int opt;
    int unif_flag = 1;  // use uniform packets if 1, exp if 0
    int num_src = -1;
    int num_pkt = -1;
    int depth = -1;
    int mean = -1;
    char *lock_type = NULL;
    char *strategy = NULL;
    int seed = 0;

    while ((opt = getopt(argc, argv, "n:t:d:w:u:l:s:e:")) != -1) {
        switch(opt) {
            case 'n':
                num_src = atoi(optarg);
                break;
            case 't':
                num_pkt = atoi(optarg);
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

    if (num_src == -1 || num_pkt == -1 || mean == -1) {
        printf("Usage: \n./driver_output -t NUM_PACKETS -n NUM_SOURCES -w MEAN "
        "-u [1|0] -d DEPTH -l [clh|mutex] -s [lockfree|homequeue|awesome] "
        "-e seed\n"
        "Serial example: ./driver_output -t 1000 -n 7 -w 1000 -u 1\n"
        "Parallel example: ./driver_output -t 1000 -n 7 -w 1000 -u 1 -d 8 "
        "-l clh -s homequeue\n");
        exit(1);
    }
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

    // run serial or parallel
    int serial_flag = 1;
    long global_ret = 0;  // accumulated checksum

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
    if (serial_flag) {
        global_ret = serial(source, get_packet, num_src, num_pkt);
    } else {
        shared_t *shared = init_shared(num_src, num_pkt, depth, lock_type);
        void *(*worker_strategy)(void *) = NULL;

        if (strcmp(strategy, "lockfree") == 0) {
            worker_strategy = &worker_lockfree_output;
        } else if (strcmp(strategy, "homequeue") == 0) {
            if (strcmp(lock_type, "mutex") == 0) {
                worker_strategy = &worker_homequeue_mutex_output;
            } else if (strcmp(lock_type, "clh") == 0) {
                worker_strategy = &worker_homequeue_clh_output;
            }
        } else if (strcmp(strategy, "awesome") == 0) {
            if (strcmp(lock_type, "mutex") == 0) {
                worker_strategy = &worker_awesome_mutex_output;
            } else if (strcmp(lock_type, "clh") == 0) {
                worker_strategy = &worker_awesome_clh_output;
            }
        } else {
            printf("Invalid strategy: %s.\n", strategy);
            exit(1);
        }
        global_ret = parallel(source, get_packet, worker_strategy, shared);
        destroy_shared(shared);
    }
    printf("Time: %0.3lf\n", getElapsedTime(&watch));

    // clean up
    deletePacketSource(source);

    // write the global checksum to file
    char *fname = calloc(30, sizeof(char));
    if (serial_flag) {
        strcat(fname, "serial_res.txt");
    } else {
        if (strcmp(strategy, "lockfree") == 0) {
            strcat(fname, "lockfree_res.txt");
        } else {
            strcat(fname, lock_type);
            strcat(fname, "_");
            strcat(fname, strategy);
            strcat(fname, "_res.txt");  // ex. mutex_homequeue_res.txt
        }    
    }
    FILE *fout = fopen(fname, "w");
    if (fout == NULL) {
        printf("Error writing to file.\n");
        exit(1);
    }
    fprintf(fout, "%ld\n", global_ret);
    printf("Output the total checksum %ld to %s\n", global_ret, fname);
    free(fname);

    return 0;
}

void *worker_lockfree_output(void *args) {
    long finger;
    long finger_sum = 0;
    int rc;
    volatile Packet_t *pkt = NULL;

    targs_t *targs = args;
    int tid = targs->tid;
    shared_t *shared = targs->shared;
    lamport_queue_t *my_queue = shared->all_queues[tid];

    while (1) {
        pkt = (Packet_t *) dequeue(my_queue);
        if (pkt != NULL) {
            finger = getFingerprint(pkt->iterations, pkt->seed);
            finger_sum += finger;
        } else {
            if (timeout_flag == 1) {
                break;  // dispatcher is done
            }
            rc = sched_yield();
        }
    }
    rc--;  // turn off compiler warning
    pthread_exit((void *) finger_sum);
}

/* Begin Worker Output Functions */
void *worker_homequeue_mutex_output(void *args) {
    long finger;
    long finger_sum = 0;
    int rc;
    volatile Packet_t *pkt = NULL;

    targs_t *targs = args;
    int tid = targs->tid;
    shared_t *shared = targs->shared;
    lamport_queue_t *my_queue = shared->all_queues[tid];
    pthread_mutex_t *my_mutex = shared->all_locks[tid];

    while (1) {
        pthread_mutex_lock(my_mutex);
        pkt = (Packet_t *) dequeue(my_queue);
        pthread_mutex_unlock(my_mutex);
        if (pkt != NULL) {
            finger = getFingerprint(pkt->iterations, pkt->seed);
            finger_sum += finger;  // record total
        } else {
            if (timeout_flag == 1) {
                break;
            }
            rc = sched_yield();  // continue         
        }
    }
    rc--;  // turn off compiler warning
    pthread_exit((void *) finger_sum);
}

void *worker_lastqueue_mutex_output(void *args) {
    long finger;
    long finger_sum = 0;
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

    while (1) {
        // first try the direct mapping queue, if empty, try the last one
        pthread_mutex_lock(my_mutex);
        pkt = (Packet_t *) dequeue(my_queue);
        pthread_mutex_unlock(my_mutex);
        if (pkt != NULL) {
            finger = getFingerprint(pkt->iterations, pkt->seed);
            finger_sum += finger;  // record checksum total
        } else {
            if (timeout_flag == 1) {
                break;
            }
            idx = last_index;
            rc = pthread_mutex_trylock(mutexes[idx]);
            if (rc == 0) {
                last_index = (idx + 1) & (num_src - 1);
                pkt = (Packet_t *) dequeue(queues[idx]);
                pthread_mutex_unlock(mutexes[idx]);
                if (pkt != NULL) {
                    // possible that some one dequeues the last item enqueued
                    // by the dispatcher before this thread acquires the lock
                    finger = getFingerprint(pkt->iterations, pkt->seed);
                    finger_sum += finger;
                }
            }
            rc = sched_yield();
        }
    }
    rc--;  // turn off compiler warning
    pthread_exit((void *) finger_sum);
}

void *worker_homequeue_clh_output(void *args) {
    long finger;
    long finger_sum = 0;
    int rc;
    volatile Packet_t *pkt = NULL;

    targs_t *targs = args;
    int tid = targs->tid;
    shared_t *shared = targs->shared;

    lamport_queue_t *my_queue = shared->all_queues[tid];
    clh_t *my_clh = shared->all_locks[tid];
    volatile qnode_t *my_node = qnode_init(1);
    volatile qnode_t *my_pred = NULL;

    while (1) {
        my_pred = clh_lock(my_clh, my_node, my_pred);
        pkt = (Packet_t *) dequeue(my_queue);
        my_node = clh_unlock(my_clh, my_node, my_pred);
        if (pkt != NULL) {
            finger = getFingerprint(pkt->iterations, pkt->seed);
            finger_sum += finger;  // record total
        } else {
            if (timeout_flag == 1) {
                break;
            }
            rc = sched_yield();
        }
    }
    rc--;  // turn off compiler warning
    pthread_exit((void *) finger_sum);
}

void *worker_lastqueue_clh_output(void *args) {
    long finger;
    long finger_sum = 0;
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

    while (1) {
        // first try the direct mapping queue, if empty, try the last one
        my_pred = clh_lock(my_clh, my_node, my_pred);
        pkt = (Packet_t *) dequeue(my_queue);
        my_node = clh_unlock(my_clh, my_node, my_pred);
        if (pkt != NULL) {
            finger = getFingerprint(pkt->iterations, pkt->seed);
            finger_sum += finger;  // record checksum total
        } else {
            if (timeout_flag == 1) {
                break;
            }
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
                    finger_sum += finger;
                }
            }
            rc = sched_yield();
        }
    }
    rc--;  // turn off compiler warning
    pthread_exit((void *) finger_sum);
}
/* End Worker Output Functions */