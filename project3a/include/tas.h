#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    volatile int state;
} tas_t;

// int return type in accordance with pthread_mutex
// use NULL for unused too keep syntax consistent with pthread_mutex_init
int tas_init(tas_t *tas, __attribute__((unused)) int unused);

int tas_lock(tas_t *tas);

// return 0 if lock is acquired and 1 if lock is busy
int tas_trylock(tas_t *tas);

int tas_unlock(tas_t *tas);

int tas_destroy(tas_t *tas);