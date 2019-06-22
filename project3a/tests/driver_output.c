#include "driver_output.h" 

StopWatch_t watch;

int main(int argc, char **argv) {
    int opt;
    int big = 0;
    int num_threads = 0;
    char *lock_type = NULL;  // tas, ttas, alock, clh, mutex

    while ((opt = getopt(argc, argv, "b:n:l:")) != -1) {
        switch(opt) {
            case 'b':
                big = atoi(optarg);
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
            "Serial example: ./driver_output -b 1000000 -n 0\n"
            "Parallel example: ./driver_output -b 1000000 -n 4 -l tas\n");
        exit(1);
    }

    int *counter = calloc(1, sizeof(int));  // create counter

    // correctness output file for both serial and parallel
    int *nums = calloc(big + 10, sizeof(int));  // protect against counting over
    char *fres = calloc(16, sizeof(char));
    lock_type == NULL ? strcat(fres, "serial") : strcat(fres, lock_type);   
    strcat(fres, "_res.txt");  // [serial|tas]_res.txt
    printf("Correctness output: %s\n", fres);

    // run serial or parallel
    if (num_threads == 0) {
        output_serial(counter, big, nums);
    } else {
        // create fairness output file for parallel only
        int *counts = calloc(num_threads, sizeof(int));
        char *ffifo = calloc(16, sizeof(char));
        strcat(ffifo, lock_type);
        strcat(ffifo, "_fifo.txt");  // tas_fifo.txt
        printf("Fairness output: %s\n", ffifo);

        // get the specified type of lock and its utils, and run parallel
        void *(*worker_func)(void *);
        worker_func = &output_worker_generic;

        if (strcmp(lock_type, "alock") == 0) {
            worker_func = &output_worker_alock;
        } else if (strcmp(lock_type, "clh") == 0) {
            worker_func = &output_worker_clh;
        }
        
        lock_utils_t *lock_utils = lock_utils_init(lock_type);
        output_parallel_generic(counter, big, num_threads, nums, counts, lock_utils, 
            worker_func);

        // write counts to file
        output_array(ffifo, counts, num_threads);
        free(ffifo);
        free(counts);
    }
    printf("Time: %0.3f\n", getElapsedTime(&watch));
    printf("Final Counter: %d\n", *counter);
    printf("Writing to file...\n");

    // write nums to file
    output_array(fres, nums, big + 10);
    free(fres);
    free(nums);

    return 0;
}

void output_serial(int *counter, int big, int *nums) {
    volatile int *v_counter = counter;
    startTimer(&watch);
    for (int i = 0; i < big; i++) {
        *v_counter += 1;
        nums[*counter - 1] += 1;
    }
    stopTimer(&watch);
}

/* Start Parallel and Worker Functions */

void output_parallel_generic(int *counter, int big, int num_threads, int *nums, 
    int *counts, lock_utils_t *lock_utils, void *(*worker_func)(void *)) {

    int rc, t;
    pthread_t threads[num_threads];
    output_targs_t ttargs_arr[num_threads];
    output_shared_t *shared = calloc(1, sizeof(output_shared_t));

    int rounded_n = next_pow2(num_threads);
    printf("Rounded n = %d to %d for ALock's size\n", num_threads, rounded_n);
    lock_utils->lock_init(lock_utils->lock, rounded_n);
    shared->counter = counter;
    shared->big = big;
    shared->nums = nums;
    shared->counts = counts;
    shared->lock_utils = lock_utils;

    startTimer(&watch);
    for (t = 0; t < num_threads; t++) {
        ttargs_arr[t].tid = t;  // unique, all other fields shared
        ttargs_arr[t].shared = shared;
        rc = pthread_create(&threads[t], NULL, worker_func, 
            (void *)&ttargs_arr[t]);
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

void *output_worker_generic(void *args) {
    output_targs_t *ttargs = args;
    output_shared_t *shared = ttargs->shared;
    lock_utils_t *lock_utils = shared->lock_utils;
    int *counter = shared->counter;
    int *nums = shared->nums;
    int counts = 0;
    int break_flag = 0;  // if true, release the lock and exit
    
    while (1) {
        lock_utils->lock_lock(lock_utils->lock);
        if (*counter < shared->big) {
            *counter += 1;
            nums[*counter - 1] += 1;  // record counter
            counts += 1;
        } else {
            break_flag = 1;
        }
        lock_utils->lock_unlock(lock_utils->lock);
        if (break_flag) {
            break;
        }
    }
    shared->counts[ttargs->tid] = counts; // record increments
    pthread_exit(0);
}

void *output_worker_clh(void *args) {
    output_targs_t *ttargs = args;
    output_shared_t *shared = ttargs->shared;
    lock_utils_t *lock_utils = shared->lock_utils;
    clh_t *clh = lock_utils->lock;

    int *counter = shared->counter;
    int *nums = shared->nums;
    int counts = 0;
    int break_flag = 0;

    // thread local nodes
    volatile qnode_t *my_node = qnode_init(1);
    volatile qnode_t *my_pred = NULL;
    
    while (1) {
        my_pred = clh_lock(clh, my_node, my_pred);
        if (*counter < shared->big) {
            *counter += 1;
            nums[*counter - 1] += 1;
            counts += 1;
        } else {
            break_flag = 1;
        }       
        my_node = clh_unlock(clh, my_node, my_pred);
        if (break_flag) {
            break;
        }
    }
    shared->counts[ttargs->tid] = counts;
    pthread_exit(0);
}

void *output_worker_alock(void *args) {
    output_targs_t *ttargs = args;
    output_shared_t *shared = ttargs->shared;
    lock_utils_t *lock_utils = shared->lock_utils;
    alock_t *alock = lock_utils->lock;

    int *counter = shared->counter;
    int *nums = shared->nums;
    int counts = 0;
    int break_flag = 0;

    // thread local slot
    int my_slot = 0;

    while (1) {
        my_slot = alock_lock(alock, my_slot);
        if (*counter < shared->big) {
            *counter += 1;
            nums[*counter - 1] += 1;
            counts += 1;
        } else {
            break_flag = 1;
        }
        alock_unlock(alock, my_slot);
        if (break_flag) {
            break;
        }
    }
    shared->counts[ttargs->tid] = counts;
    pthread_exit(0);
}

/* End Parallel and Worker Functions */

void output_array(char *fname, int *arr, int length) {
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