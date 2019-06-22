#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "lamport_queue.h"
#include "packetsource.h"
#include "fingerprint.h"
#include "stopwatch.h"

// all threads have access to write to output array
typedef struct {
    int num_pkt; // current row
    int cur_src; // current col
    lamport_queue_t *queue;
    long **res;
} outargs_t;

void serial(long **res, PacketSource_t *src, 
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt);

void parallel(long **res, PacketSource_t *source, 
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt, int depth);

void *worker_func(void *targs);

void serial_queue(long **res, PacketSource_t *source, 
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt, int depth);

// helpers for correctness testing only
long **init_arr(int num_row, int num_col);

int output(char *fname, long **res, int num_row, int num_col);