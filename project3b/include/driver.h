#include "common.h"
#define worker_awesome_mutex worker_lastqueue_mutex
#define worker_awesome_clh worker_lastqueue_clh

void *worker_lockfree(void *args);

void *worker_homequeue_mutex(void *args);

void *worker_lastqueue_mutex(void *args);

void *worker_homequeue_clh(void *args);

void *worker_lastqueue_clh(void *args);