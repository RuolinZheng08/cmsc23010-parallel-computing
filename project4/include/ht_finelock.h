#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "hashtable.h"

typedef struct {
    int rwlocks_len;  // length of rwlock array
    int rwlocks_mask;
    SerialHashTable_t *ht_serial;
    pthread_rwlock_t **rwlocks;
} ht_finelock_t;

ht_finelock_t *ht_finelock_init(int log_size, int max_bucket_size);

// need this because one resize could move buckets around and still results
// in some buckets exceeding max_bucket_size
void ht_finelock_resize_if_necessary(ht_finelock_t *ht_finelock, int key);

void ht_finelock_add(ht_finelock_t *ht_finelock, int key, volatile Packet_t *x);

bool ht_finelock_remove(ht_finelock_t *ht_finelock, int key);

bool ht_finelock_contains(ht_finelock_t *ht_finelock, int key);

void ht_finelock_resize(ht_finelock_t *ht_finelock);

// optimistic
bool ht_optimistic_contains(ht_finelock_t *ht_finelock, int key);