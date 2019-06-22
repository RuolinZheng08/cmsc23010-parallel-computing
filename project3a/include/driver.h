#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "lock_utils.h"
#include "stopwatch.h"

// arguments shared across threads
typedef struct {
    long *counter;
    long big;
    lock_utils_t *lock_utils;
} shared_t;

// thread arguments for testing
typedef struct {
    int tid;  // unique, all other fields shared
    shared_t *shared;
} targs_t;

void serial(long *counter, long big);

// lock_utils provides the same interface for mutex, tas, and ttas
void parallel_generic(long *counter, long big, int num_threads, 
    lock_utils_t *lock_utils, void *(*func)(void *));

void *worker_generic(void *args);

void *worker_clh(void *args);

void *worker_alock(void *args);

unsigned int next_pow2(unsigned int n);