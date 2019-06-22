#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "hashgenerator.h"
#include "hashtable.h"
#include "ht_finelock.h"  // both finelock and optimistic

// ht_utils_t provide the interface of add, remove, and contains
// for serial, finelock, and optimistic
typedef struct {
    void *hashtable;
    void (*ht_add)(void *, int, volatile Packet_t *);
    bool (*ht_remove)(void *, int);
    bool (*ht_contains)(void *, int);
} ht_utils_t;

// package the correct hashtable type with its utils
ht_utils_t *ht_utils_init(char *ht_type, int log_size, int max_bkt_size);

// apply the operation specified in the hash packet to the hash table
// and return the return code for remove and contains
bool ht_utils_apply_operation(ht_utils_t *ht_utils, HashPacket_t *pkt);