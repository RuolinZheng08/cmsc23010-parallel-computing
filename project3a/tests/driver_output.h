#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "lock_utils.h"
#include "stopwatch.h"

// arguments shared across threads
typedef struct {
    int *counter;
    int big;
    int *nums;
    int *counts;
    lock_utils_t *lock_utils;
} output_shared_t;

// thread arguments for testing
typedef struct {
    int tid;  // unique, all other fields shared
    output_shared_t *shared;
} output_targs_t;

// both serial and parallel write in-place to their array arguments
void output_serial(int *counter, int big, int *nums);

// lock_utils provides the same interface for mutex, tas, and ttas
void output_parallel_generic(int *counter, int big, int num_threads, int *nums, 
    int *counts, lock_utils_t *lock_utils, void *(*func)(void *));

void *output_worker_generic(void *args);

void *output_worker_clh(void *args);

void *output_worker_alock(void *args);

// helper functions
void output_array(char *fname, int *arr, int length);

unsigned int next_pow2(unsigned int n);