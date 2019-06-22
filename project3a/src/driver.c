#include "driver.h" 

StopWatch_t watch;

int main(int argc, char **argv) {
    int opt;
    long big = 0;
    int num_threads = 0;
    char *lock_type = NULL;  // tas, ttas, alock, clh, mutex

    while ((opt = getopt(argc, argv, "b:n:l:")) != -1) {
        switch(opt) {
            case 'b':
                big = atol(optarg);
                break;
            case 'n':
                num_threads = atoi(optarg);
                break;
            case 'l':
                lock_type = strdup(optarg);
                break;
        }
    }

    if (big == 0 || num_threads < 0 || (num_threads > 0 && lock_type == NULL)) {
        printf("Usage:"
            "./driver -b BIG -n NUM_THREADS -l [tas|ttas|alock|clh|mutex]\n\n"
            "Serial example: ./driver -b 100000000 -n 0\n"
            "Parallel example: ./driver -b 100000000 -n 4 -l tas\n");
        exit(1);
    }

    long *counter = calloc(1, sizeof(long));  // create counter

    // run serial or parallel
    if (num_threads == 0) {
        serial(counter, big);
    } else {

        // get the specified type of lock and its utils, and run parallel
        void *(*worker_func)(void *);
        worker_func = &worker_generic;

        if (strcmp(lock_type, "alock") == 0) {
            worker_func = &worker_alock;
        } else if (strcmp(lock_type, "clh") == 0) {
            worker_func = &worker_clh;
        }
        
        lock_utils_t *lock_utils = lock_utils_init(lock_type);
        parallel_generic(counter, big, num_threads, lock_utils, worker_func);
    }
    printf("Time: %f\n", getElapsedTime(&watch));
    printf("Final Counter: %ld\n", *counter);

    return 0;
}

void serial(long *counter, long big) {
    volatile long *v_counter = counter;
    startTimer(&watch);
    for (int i = 0; i < big; i++) {
        *v_counter += 1;
    }
    stopTimer(&watch);
}

/* Start Parallel and Worker Functions */

void parallel_generic(long *counter, long big, int num_threads, 
    lock_utils_t *lock_utils, void *(*worker_func)(void *)) {

    int rc, t;
    pthread_t threads[num_threads];
    targs_t targs_arr[num_threads];
    shared_t *shared = calloc(1, sizeof(shared_t));

    int rounded_n = next_pow2(num_threads);
    printf("Rounded n = %d to %d for ALock's size\n", num_threads, rounded_n);
    lock_utils->lock_init(lock_utils->lock, rounded_n);
    shared->counter = counter;
    shared->big = big;
    shared->lock_utils = lock_utils;

    startTimer(&watch);
    for (t = 0; t < num_threads; t++) {
        targs_arr[t].tid = t;  // unique, all other fields shared
        targs_arr[t].shared = shared;
        rc = pthread_create(&threads[t], NULL, worker_func, 
            (void *)&targs_arr[t]);
        if (rc != 0) {
            printf("Error creating thread.");
        }
    }

    for (t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
    stopTimer(&watch);
    lock_utils_destroy(lock_utils);
}

void *worker_generic(void *args) {
    targs_t *targs = args;
    shared_t *shared = targs->shared;
    lock_utils_t *lock_utils = shared->lock_utils;
    long *counter = shared->counter;
    int break_flag = 0;
    
    while (1) {
        lock_utils->lock_lock(lock_utils->lock);
        if (*counter < shared->big) {
            *counter += 1;
        } else {
            break_flag = 1;
        }
        lock_utils->lock_unlock(lock_utils->lock);
        if (break_flag) {
            break;
        }
    }
    pthread_exit(0);
}

void *worker_clh(void *args) {
    targs_t *targs = args;
    shared_t *shared = targs->shared;
    lock_utils_t *lock_utils = shared->lock_utils;
    clh_t *clh = lock_utils->lock;
    long *counter = shared->counter;
    int break_flag = 0;

    // thread local nodes
    volatile qnode_t *my_node = qnode_init(1);
    volatile qnode_t *my_pred = NULL;
    
    while (1) {
        my_pred = clh_lock(clh, my_node, my_pred);
        if (*counter < shared->big) {
            *counter += 1;
        } else {
            break_flag = 1;
        }
        my_node = clh_unlock(clh, my_node, my_pred);
        if (break_flag) {
            break;
        }
    }
    pthread_exit(0);
}

void *worker_alock(void *args) {
    targs_t *targs = args;
    shared_t *shared = targs->shared;
    lock_utils_t *lock_utils = shared->lock_utils;
    alock_t *alock = lock_utils->lock;
    long *counter = shared->counter;
    int break_flag = 0;

    // thread local slot
    int my_slot = 0;

    while (1) {
        my_slot = alock_lock(alock, my_slot);
        if (*counter < shared->big) {
            *counter += 1;
        } else {
            break_flag = 1;
        }
        alock_unlock(alock, my_slot);
        if (break_flag) {
            break;
        }
    }
    pthread_exit(0);
}

/* End Parallel and Worker Functions */

void output(char *fname, int *arr, int length) {
    FILE *fout = fopen(fname, "w");
    if (fout == NULL) {
        printf("Error writing to %s. \n", fname);
        exit(1);
    }
    for (int i = 0; i < length; i++) {
        fprintf(fout, "%d\n", arr[i]);
    }
    fclose(fout);
}

// reference: http://locklessinc.com/articles/next_pow2/
unsigned int next_pow2(unsigned int n) {
    n--;
    n |= (n >> 1);
    n |= (n >> 1);
    n |= (n >> 1);
    n |= (n >> 1);
    n |= (n >> 1);
    return n + 1;
}