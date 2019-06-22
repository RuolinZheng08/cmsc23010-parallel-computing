#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "paddedprim.h"

typedef struct {
    int size;
    volatile int tail;
    PaddedPrimBool_t *flags;
} alock_t;

int alock_init(alock_t *alock, int num_threads);

int alock_lock(alock_t *alock, int my_slot);

// return -1 if lock is busy, and my_slot if acquired
int alock_trylock(alock_t *alock, int my_slot);

int alock_unlock(alock_t *alock, int my_slot);

int alock_destroy(alock_t *alock);