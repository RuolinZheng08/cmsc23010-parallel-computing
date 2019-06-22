#include "tas.h"

int tas_init(tas_t *tas, __attribute__((unused)) int unused) {
    tas->state = 0;
    return 0;
}

int tas_lock(tas_t *tas) {
    while (__sync_lock_test_and_set(&(tas->state), 1)) {
        // spin
    }
    return 0;
}

int tas_trylock(tas_t *tas) {
    if (tas->state == 1) {
        return 1;  // lock is busy
    } else {
        while (__sync_lock_test_and_set(&(tas->state), 1)) {
            // spin
        }
        return 0;  // successfully acquired the lock
    }
}

int tas_unlock(tas_t *tas) {
    tas->state = 0;
    return 0;
}

int tas_destroy(tas_t *tas) {
    free(tas);
    return 0;
}