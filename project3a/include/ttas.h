#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    volatile int state;
} ttas_t;

int ttas_init(ttas_t *ttas, __attribute__((unused)) int unused);

int ttas_lock(ttas_t *ttas);

int ttas_trylock(ttas_t *ttas);

int ttas_unlock(ttas_t *ttas);

int ttas_destroy(ttas_t *ttas);