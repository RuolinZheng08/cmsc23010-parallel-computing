#include "ttas.h"

int ttas_init(ttas_t *ttas, __attribute__((unused)) int unused) {
    ttas->state = 0;
    return 0;
}

int ttas_lock(ttas_t *ttas) {
    while (1) {
        while (ttas->state) {
            // spin
        }
        if (!__sync_lock_test_and_set(&(ttas->state), 1)) {
            return 0;
        }
    }
}

int ttas_trylock(ttas_t *ttas) {
    if (ttas->state == 1) {
        return 1;
    } else {
        while (1) {
            while (ttas->state) {
                // spin
            }
            if (!__sync_lock_test_and_set(&(ttas->state), 1)) {
                return 0;
            }
        }
    }
}

int ttas_unlock(ttas_t *ttas) {
    ttas->state = 0;
    return 0;
}

int ttas_destroy(ttas_t *ttas) {
    free(ttas);
    return 0;
}