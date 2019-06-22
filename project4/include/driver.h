#include <sched.h>  // for sched_yield()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "hashgenerator.h"
#include "fingerprint.h"
#include "stopwatch.h"
#include "lamport_queue.h"
#include "ht_utils.h"

#define depth 8
#define log_size 4
#define max_bkt_size 4

typedef struct {
    int num_workers;
    lamport_queue_t **all_queues;
    void **all_locks;
    ht_utils_t *ht_utils;
} shared_t;

typedef struct {
    int tid;
    shared_t *shared;
} targs_t;

long serial(HashPacketGenerator_t *source, ht_utils_t *ht_utils);

// helper functions for parallel
shared_t *init_shared(ht_utils_t *ht_utils, int num_workers);

// timer thread
void *timer_func(void *args);

// worker_lastqueue_mutex for parallel
// worker_drop for parallel no load, shared->ht_utils == NULL
long parallel(HashPacketGenerator_t *source, shared_t *shared);

void *worker_lastqueue_mutex(void *args);

void *worker_drop(void *args);