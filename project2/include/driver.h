#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "lamport_queue.h"
#include "packetsource.h"
#include "fingerprint.h"
#include "stopwatch.h"

typedef struct {
    int num_pkt;
    lamport_queue_t *queue;
} targs_t;

void serial(PacketSource_t *src, 
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt);

void parallel(PacketSource_t *source, 
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt, int depth);

void *worker_func(void *targs);

void serial_queue(PacketSource_t *source, 
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt, int depth);