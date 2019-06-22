#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <criterion/criterion.h>
#include "alock.h"
#include "paddedprim.h"

typedef struct {
    int *counter;
    alock_t *alock;
} alock_targs_t;

Test(alock, init) {
    alock_t *alock = calloc(1, sizeof(alock_t));
    alock_init(alock, 4);
    cr_expect(alock->tail == 0);
    cr_expect(alock->size == 4);
    cr_expect(alock->flags[0].value == 1);
    for (int i = 1; i < alock->size; i++) {
        cr_expect(alock->flags[i].value == 0);
    }
}

Test(alock, lock_and_unlock) {
    alock_t *alock = calloc(1, sizeof(alock_t));
    alock_init(alock, 2);
    int my_slot = 0;

    my_slot = alock_lock(alock, my_slot);
    cr_expect(my_slot == 0);
    cr_expect(alock->flags[my_slot].value == 1);
    cr_expect(alock->tail == 1);
    cr_expect(alock->flags[alock->tail].value == 0);

    alock_unlock(alock, my_slot);
    cr_expect(alock->flags[my_slot].value == 0);
    cr_expect(alock->flags[(my_slot + 1) % alock->size].value == 1);

    my_slot = alock_lock(alock, my_slot);
    cr_expect(my_slot == 1);
    cr_expect(alock->flags[my_slot].value == 1);
    cr_expect(alock->tail == 2);
    cr_expect(alock->flags[alock->tail % alock->size].value == 0);

    alock_unlock(alock, my_slot);
    cr_expect(alock->flags[my_slot].value == 0);
    cr_expect(alock->flags[(my_slot + 1) % alock->size].value == 1);
}