#include "alock.h"

int alock_init(alock_t *alock, int num_threads) {
    alock->size = num_threads;
    alock->tail = 0;
    alock->flags = calloc(alock->size, sizeof(PaddedPrimBool_t));
    alock->flags[0].value = 1;
    for (int i = 1; i < alock->size; i++) {
        alock->flags[i].value = 0;
    }
    return 0;
}

int alock_lock(alock_t *alock, int my_slot) {
    // x % y == x & (y - 1)
    my_slot = __sync_fetch_and_add(&alock->tail, 1) & (alock->size - 1);
    while (!alock->flags[my_slot].value) {
        // spin
    }
    return my_slot;
}

int alock_trylock(alock_t *alock, int my_slot) {
    my_slot = (alock->tail + 1) & (alock->size - 1);
    if (alock->flags[my_slot].value) {
        return -1;
    } else {
        my_slot = __sync_fetch_and_add(&alock->tail, 1) & (alock->size - 1);
        while (!alock->flags[my_slot].value) {
            // spin
        }
        return my_slot;
    }
}

int alock_unlock(alock_t *alock, int my_slot) {
    alock->flags[my_slot].value = 0;
    alock->flags[(my_slot + 1) & (alock->size - 1)].value = 1;
    return 0;
}

int alock_destroy(alock_t *alock) {
    free((void *) alock->flags);
    free(alock);
    return 0;
}