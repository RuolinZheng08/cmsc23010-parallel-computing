#include "common.h"  // reuse common functions

#define worker_awesome_mutex_output worker_lastqueue_mutex_output
#define worker_awesome_clh_output worker_lastqueue_clh_output

void *worker_lockfree_output(void *args);

void *worker_homequeue_mutex_output(void *args);

void *worker_lastqueue_mutex_output(void *args);

void *worker_homequeue_clh_output(void *args);

void *worker_lastqueue_clh_output(void *args);