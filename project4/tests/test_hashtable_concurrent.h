#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>  // for correctness testing
#include <pthread.h>
#include "hashgenerator.h"
#include "fingerprint.h"
#include "ht_utils.h"

#define fraction_add 0.45
#define fraction_remove 0.05
#define hit_rate 0.8
#define mean 100

#define log_size 4
#define max_bkt_size 32
#define num_pkts 40000
#define num_workers 4
#define chunk_size num_pkts / num_workers

typedef struct {
    int tid;
    int chunk;
    HashPacket_t **pkts;  // an array of packets to add into the hashtable
    ht_utils_t *ht_utils;
} testtargs_t;

// one source, one packet array, two hashtables
// compare the total checksum from serial and parallel
void test_generic(HashPacket_t * (*get_packet1)(HashPacketGenerator_t *source),
    HashPacket_t * (*get_packet2)(HashPacketGenerator_t *source), int preload, bool optimistic);

// get_dummy_packet could be NULL
HashPacket_t **gen_packets_arr(HashPacketGenerator_t *source, 
    HashPacket_t * (*get_valid_packet)(HashPacketGenerator_t *source),
    HashPacket_t * (*get_dummy_packet)(HashPacketGenerator_t *source));

// check whether this_pkt's key clashes with any of the existing ones in the array
bool key_clash(HashPacket_t **pkts, HashPacket_t *this_pkt);

// t0 and t1 get valid packets; t2 and t3 get dummy packets
HashPacket_t **gen_valid_and_dummy(HashPacketGenerator_t *source, 
    HashPacket_t * (*get_valid_packet)(HashPacketGenerator_t *source),
    HashPacket_t * (*get_dummy_packet)(HashPacketGenerator_t *source));

// if rc = ht_utils_apply_operation(ht_utils, pkt) is true (success), 
// accumulate the checksum; else, pass
long serial_generic(ht_utils_t *ht_utils, HashPacket_t **pkts);

void *worker_generic(void *args);

long parallel_generic(ht_utils_t *ht_utils, HashPacket_t **pkts);