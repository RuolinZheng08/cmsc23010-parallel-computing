#include <sched.h>  // for sched_yield()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include "packetsource.h"
#include "fingerprint.h"
#include "stopwatch.h"
#include "lamport_queue.h"
#include "clh.h"

typedef struct {
    int num_pkt;  // used only in driver_output.c
    int num_src;
    lamport_queue_t **all_queues;
    void **all_locks;
} shared_t;

typedef struct {
    int tid;
    shared_t *shared;
} targs_t;

long serial(PacketSource_t *src, volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt);

// helper functions for parallel
shared_t *init_shared(int num_src, int num_pkt, int depth, char *lock_type);

void destroy_shared(shared_t *shared);

// timer thread
void *timer_func(void *args);

long parallel(PacketSource_t *source,
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    void *(*worker_strategy)(void *), shared_t *shared);