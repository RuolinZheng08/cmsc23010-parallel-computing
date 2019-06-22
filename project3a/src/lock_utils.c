#include "lock_utils.h"

int wrapper_mutex_init(pthread_mutex_t *mutex, __attribute__((unused)) int unused) {
    return pthread_mutex_init(mutex, NULL);
}

lock_utils_t *lock_utils_init(char *lock_type) {
    lock_utils_t *lock_utils = calloc(1, sizeof(lock_utils_t));

    if (strcmp(lock_type, "tas") == 0) {

        lock_utils->lock = calloc(1, sizeof(tas_t));
        lock_utils->lock_init = (int (*)(void *, int)) &tas_init;
        lock_utils->lock_lock = (int (*)(void *)) &tas_lock;
        lock_utils->lock_trylock = (int (*)(void *)) &tas_trylock;
        lock_utils->lock_unlock = (int (*)(void *)) &tas_unlock;
        lock_utils->lock_destroy = (int (*)(void *)) &tas_destroy;

    } else if (strcmp(lock_type, "ttas") == 0) {

        lock_utils->lock = calloc(1, sizeof(ttas_t));
        lock_utils->lock_init = (int (*)(void *, int)) &ttas_init;
        lock_utils->lock_lock = (int (*)(void *)) &ttas_lock;
        lock_utils->lock_trylock = (int (*)(void *)) &ttas_trylock;
        lock_utils->lock_unlock = (int (*)(void *)) &ttas_unlock;
        lock_utils->lock_destroy = (int (*)(void *)) &ttas_destroy;

    } else if (strcmp(lock_type, "alock") == 0) {

        lock_utils->lock = calloc(1, sizeof(alock_t));
        lock_utils->lock_init = (int (*)(void *, int)) &alock_init;
        lock_utils->lock_destroy = (int (*)(void *)) &alock_destroy;

    } else if (strcmp(lock_type, "clh") == 0) {

        lock_utils->lock = calloc(1, sizeof(clh_t));
        lock_utils->lock_init = (int (*)(void *, int)) &clh_init;
        lock_utils->lock_destroy = (int (*)(void *)) &clh_destroy;

    } else if (strcmp(lock_type, "mutex") == 0) {

        lock_utils->lock = calloc(1, sizeof(pthread_mutex_t));
        lock_utils->lock_init = (int (*)(void *, int)) &wrapper_mutex_init;
        lock_utils->lock_lock = (int (*)(void *)) &pthread_mutex_lock;
        lock_utils->lock_trylock = (int (*)(void *)) &pthread_mutex_trylock;
        lock_utils->lock_unlock = (int (*)(void *)) &pthread_mutex_unlock;
        lock_utils->lock_destroy = (int (*)(void *)) &pthread_mutex_destroy;

    } else {
        printf("Invalid Lock Type.\n");
        exit(1);
    }
    return lock_utils;
}

void lock_utils_destroy(lock_utils_t *lock_utils) {
    lock_utils->lock_destroy(lock_utils->lock);
    free(lock_utils);
}