#include "driver.h"

StopWatch_t watch;
int timeout_flag = 0;
int last_index = 0;

int main(int argc, char **argv) {
    int opt;

    // serial
    long num_msec = -1;
    float fraction_add = -1;
    float fraction_remove = -1;
    float hit_rate = -1;
    long mean = -1;
    int init_size = 0;

    // parallel
    int num_workers = -1;
    char *ht_type = NULL;

    while ((opt = getopt(argc, argv, "m:a:r:h:w:s:n:t:")) != -1) {
        switch(opt) {
            case 'm':
                num_msec = atol(optarg);
                break;
            case 'a':
                fraction_add = atof(optarg);
                break;
            case 'r':
                fraction_remove = atof(optarg);
                break;
            case 'h':
                hit_rate = atof(optarg);
                break;
            case 'w':
                mean = atol(optarg);
                break;
            case 's':
                init_size = atoi(optarg);
                break;

            case 'n':
                num_workers = atoi(optarg);
                break;
            case 't':
                ht_type = strdup(optarg);
                break;
        }
    }

    if (num_msec == -1 || fraction_add == -1 || fraction_remove == -1 ||
        hit_rate == -1 || mean == -1) {
        printf("Usage: \n./driver -m NUM_MILLISECONDS -a FRACTION_ADD "
            "-r FRACTION_REMOVE -h HIT_RATE -w MEAN  -s INIT_SIZE "
            "-n NUM_WORKERS -t [finelock|optimistic]\n\n"
            "Serial example: ./driver -m 2000 -a 0.09 -r 0.01 -h 0.9 -w 4000\n"
            "Parallel example: ./driver -m 2000 -a 0.09 -r 0.01 -h 0.9 -w 4000 "
            "-n 4 -t finelock\n");
        exit(1);
    }
    printf("Parameters: -m %ld, -a %f, -r %f, -h %f, -w %ld, -s %d; -n %d, -t %s\n", 
        num_msec, fraction_add, fraction_remove, hit_rate, mean, init_size, 
        num_workers, ht_type);

    // create packet source and hash table
    HashPacketGenerator_t *source = createHashPacketGenerator(fraction_add, 
        fraction_remove, hit_rate, mean);
    ht_utils_t *ht_utils = NULL;  // remain NULL only for parallel no load
    if (!(ht_type == NULL && num_workers != -1)) {
        ht_utils = ht_utils_init(ht_type, log_size, max_bkt_size);
        // preload items
        HashPacket_t *init_pkt = NULL;
        for (int i = 0; i < init_size; i++) {
            init_pkt = getAddPacket(source);
            ht_utils->ht_add(ht_utils->hashtable, init_pkt->key, init_pkt->body);
        }
    }

    // core program
    long total_pkt;
    pthread_t timer;
    int rc;
    if (num_workers == -1) {
        // serial
        rc = pthread_create(&timer, NULL, timer_func, (void *) num_msec);
        total_pkt = serial(source, ht_utils);
        pthread_join(timer, NULL);
    } else {
        // parallel, load or no load
        shared_t *shared = init_shared(ht_utils, num_workers);
        rc = pthread_create(&timer, NULL, timer_func, (void *) num_msec);
        total_pkt = parallel(source, shared);
        pthread_join(timer, NULL);
    }
    rc--;
    printf("Total Time Handling Packets: %lf\n", getElapsedTime(&watch));
    printf("Total Packets: %ld\n", total_pkt);
}

/* Begin Serial */

long serial(HashPacketGenerator_t *source, ht_utils_t *ht_utils) {
    long global_ret = 0;
    long finger;
    bool rc;
    HashPacket_t *pkt;

    // loop until the timeout_flag is 1 in driver.c
    startTimer(&watch);
    while (timeout_flag == 0) {
        pkt = getRandomPacket(source);
        rc = ht_utils_apply_operation(ht_utils, pkt);
        finger = getFingerprint(pkt->body->iterations, pkt->body->seed);
        global_ret++;  // record the number of packet handled
    }
    stopTimer(&watch);
    finger--;  // turn off compiler warning
    rc--;
    return global_ret;
}

/* End Serial */

/* Begin Parallel Helpers */

shared_t *init_shared(ht_utils_t *ht_utils, int num_workers) {
    shared_t *shared = calloc(1, sizeof(shared_t));
    // create the queues
    lamport_queue_t **queues = calloc(num_workers, sizeof(lamport_queue_t *));
    for (int i = 0; i < num_workers; i++) {
        queues[i] = init_queue(depth);
    }
    // create the locks
    void **locks = calloc(num_workers, sizeof(pthread_mutex_t *));
    for (int i = 0; i < num_workers; i++) {
        locks[i] = calloc(1, sizeof(pthread_mutex_t));
        pthread_mutex_init(locks[i], NULL);
    }

    shared->ht_utils = ht_utils;
    shared->num_workers = num_workers;
    shared->all_queues = queues;
    shared->all_locks = locks;    
    return shared;
}

void *timer_func(void *args) {
    long num_msec = (long) args;
    struct timespec ts = {num_msec / 1000, num_msec % 1000 * 1000000L};
    nanosleep(&ts, NULL);
    timeout_flag = 1;
    pthread_exit(0);
}

/* End Parallel Helpers */

/* Begin Parallel */

long parallel(HashPacketGenerator_t *source, shared_t *shared) {
    int rc;
    int num_workers = shared->num_workers;
    lamport_queue_t **all_queues = shared->all_queues;

    void *(*worker_strategy)(void *) = worker_lastqueue_mutex;
    if (shared->ht_utils == NULL) {
        worker_strategy = worker_drop;  // parallel no load
    }

    // create threads with the specified worker strategy
    pthread_t threads[num_workers];
    targs_t targs_arr[num_workers];
    for (int i = 0; i < num_workers; i++) {
        targs_arr[i].tid = i;
        targs_arr[i].shared = shared;
        rc = pthread_create(&threads[i], NULL, worker_strategy, (void *)&targs_arr[i]);
        if (rc != 0) {
            printf("Error creating thread.\n");
            exit(1);
        }
    }

    HashPacket_t *pkt;
    // loop until the timeout_flag is 1 in driver.c
    startTimer(&watch);
    while (timeout_flag == 0) {
        for (int i = 0; i < num_workers; i++) {
            pkt = getRandomPacket(source);
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
    
    // join threads and record total number of packets processed in driver.c
    long local_ret;  // int too small to cast to void *
    long global_ret = 0;
    for (int i = 0; i < num_workers; i++) {
        pthread_join(threads[i], (void *) &local_ret);
        global_ret += local_ret;
    }

    stopTimer(&watch);

    return global_ret;
}
/* End Parallel */

/* Begin Worker Functions Definition */

void *worker_lastqueue_mutex(void *args) {
    long counter = 0;
    long finger;
    int rc;
    int idx;  // local copy of last_index

    targs_t *targs = args;
    int tid = targs->tid;
    shared_t *shared = targs->shared;
    int num_workers = shared->num_workers;

    lamport_queue_t **queues = shared->all_queues;
    lamport_queue_t *my_queue = queues[tid];
    pthread_mutex_t **mutexes = (pthread_mutex_t **) shared->all_locks;
    pthread_mutex_t *my_mutex = mutexes[tid];

    ht_utils_t *ht_utils = shared->ht_utils;

    HashPacket_t *pkt;
    while (timeout_flag == 0) {
        // first try the direct mapping queue, if empty, try the last one
        pthread_mutex_lock(my_mutex);
        pkt = (HashPacket_t *) dequeue(my_queue);
        pthread_mutex_unlock(my_mutex);
        if (pkt != NULL) {
            ht_utils_apply_operation(ht_utils, pkt);
            finger = getFingerprint(pkt->body->iterations, pkt->body->seed);
            counter++;
        } else {
            idx = last_index;
            rc = pthread_mutex_trylock(mutexes[idx]);
            if (rc == 0) {
                last_index = (idx + 1) & (num_workers - 1);  // x % y == x & (y - 1)
                pkt = (HashPacket_t *) dequeue(queues[idx]);
                pthread_mutex_unlock(mutexes[idx]);
                if (pkt != NULL) {
                    // possible that some one dequeues the last item enqueued
                    // by the dispatcher before this thread acquires the lock
                    ht_utils_apply_operation(ht_utils, pkt);
                    finger = getFingerprint(pkt->body->iterations, pkt->body->seed);
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

void *worker_drop(void *args) {
    long counter = 0;
    long finger = 0;
    int rc;
    int idx;  // local copy of last_index

    targs_t *targs = args;
    int tid = targs->tid;
    shared_t *shared = targs->shared;
    int num_workers = shared->num_workers;

    lamport_queue_t **queues = shared->all_queues;
    lamport_queue_t *my_queue = queues[tid];
    pthread_mutex_t **mutexes = (pthread_mutex_t **) shared->all_locks;
    pthread_mutex_t *my_mutex = mutexes[tid];

    HashPacket_t *pkt;
    while (timeout_flag == 0) {
        // first try the direct mapping queue, if empty, try the last one
        pthread_mutex_lock(my_mutex);
        pkt = (HashPacket_t *) dequeue(my_queue);
        pthread_mutex_unlock(my_mutex);
        if (pkt != NULL) {
            // do nothing
            counter++;
        } else {
            idx = last_index;
            rc = pthread_mutex_trylock(mutexes[idx]);
            if (rc == 0) {
                last_index = (idx + 1) & (num_workers - 1);  // x % y == x & (y - 1)
                pkt = (HashPacket_t *) dequeue(queues[idx]);
                pthread_mutex_unlock(mutexes[idx]);
                if (pkt != NULL) {
                    // do nothing
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

/* End Worker Functions Definition */