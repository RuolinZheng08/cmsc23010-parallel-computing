#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "tas.h"
#include "ttas.h"
#include "alock.h"
#include "clh.h"

// utility functions for a specific lock type
// lock_utils provides all fields for mutex, tas, and ttas
// and only the locks themselves for alock and clh
typedef struct {
    void *lock;
    int (*lock_init)(void *, int);
    int (*lock_lock)(void *);
    int (*lock_trylock)(void *);
    int (*lock_unlock)(void *);
    int (*lock_destroy)(void *);
} lock_utils_t;

// wrapper for pthread_mutex_init
int wrapper_mutex_init(pthread_mutex_t *mutex, __attribute__((unused)) int unused);

// a helper function to package the correct lock type with its utils
lock_utils_t *lock_utils_init(char *lock_type);

void lock_utils_destroy(lock_utils_t *lock_utils);