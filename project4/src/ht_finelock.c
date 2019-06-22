#include "ht_finelock.h"

ht_finelock_t *ht_finelock_init(int log_size, int max_bucket_size) {
    ht_finelock_t *ht_finelock = calloc(1, sizeof(ht_finelock_t));
    SerialHashTable_t *ht_serial = createSerialHashTable(log_size, max_bucket_size);
    ht_finelock->ht_serial = ht_serial; 

    ht_finelock->rwlocks_len = (1 << log_size);
    ht_finelock->rwlocks_mask = (1 << log_size) - 1;

    pthread_rwlock_t **rwlocks = calloc(ht_serial->size, sizeof(pthread_rwlock_t *));

    int rc;
    for (int i = 0; i < ht_serial->size; i++) {
        rwlocks[i] = calloc(1, sizeof(pthread_rwlock_t));
        rc = pthread_rwlock_init(rwlocks[i], NULL);
        if (rc != 0) {
            printf("Failed to init locks.\n");
            exit(1);
        }
    }
    ht_finelock->rwlocks = rwlocks;
    return ht_finelock;
}

void ht_finelock_resize_if_necessary(ht_finelock_t *ht_finelock, int key) {
    SerialHashTable_t *ht_serial = ht_finelock->ht_serial;
    while (ht_serial->table[key & ht_serial->mask] != NULL &&
        ht_serial->table[key & ht_serial->mask]->size >= ht_serial->maxBucketSize) {
        ht_finelock_resize(ht_finelock);
    }
}

void ht_finelock_add(ht_finelock_t *ht_finelock, int key, volatile Packet_t *x) {
    ht_finelock_resize_if_necessary(ht_finelock, key);
    int index = key & ht_finelock->rwlocks_mask;
    pthread_rwlock_t *rwlock = ht_finelock->rwlocks[index];
    int rc;
    while (pthread_rwlock_wrlock(rwlock)) {
        // yield if fails to acquire the write lock
        rc = sched_yield();
    }
    addNoCheck_ht(ht_finelock->ht_serial, key, x);
    rc = pthread_rwlock_unlock(rwlock);
    if (rc != 0) {
        printf("%s: Failed to unlock.\n", __FUNCTION__);
        exit(1);
    }
}

bool ht_finelock_remove(ht_finelock_t *ht_finelock, int key) {
    ht_finelock_resize_if_necessary(ht_finelock, key);
    int index = key & ht_finelock->rwlocks_mask;
    pthread_rwlock_t *rwlock = ht_finelock->rwlocks[index];
    int rc;
    while (pthread_rwlock_wrlock(rwlock)) {
        // yield if fails to acquire the write lock
        rc = sched_yield();
    }
    int ret;
    SerialHashTable_t *ht_serial = ht_finelock->ht_serial;
    if (ht_serial->table[key & ht_serial->mask] != NULL) {
        ret = remove_list(ht_serial->table[key & ht_serial->mask], key);
    } else {
        ret = false;
    }
    rc = pthread_rwlock_unlock(rwlock);
    if (rc != 0) {
        printf("%s: Failed to unlock.\n", __FUNCTION__);
        exit(1);
    }
    return ret;
}

bool ht_finelock_contains(ht_finelock_t *ht_finelock, int key) {
    int index = key & ht_finelock->rwlocks_mask;
    pthread_rwlock_t *rwlock = ht_finelock->rwlocks[index];
    int rc;
    while (pthread_rwlock_rdlock(rwlock)) {
        // yield if fails to acquire the read lock
        rc = sched_yield();
    }
    int ret = contains_ht(ht_finelock->ht_serial, key);
    rc = pthread_rwlock_unlock(rwlock);
    if (rc != 0) {
        printf("%s: Failed to unlock.\n", __FUNCTION__);
        exit(1);
    }
    return ret;
}

void ht_finelock_resize(ht_finelock_t *ht_finelock) {
    SerialHashTable_t *ht_serial = ht_finelock->ht_serial;
    int old_size = ht_serial->size;
    pthread_rwlock_t **rwlocks = ht_finelock->rwlocks;
    for (int i = 0; i < ht_finelock->rwlocks_len; i++) {
        while (pthread_rwlock_wrlock(rwlocks[i])) {
            // spin if fails to acquire the write lock
        }
    }
    // successfully acquired all read-write locks in order
    // check to make sure no other thread has resized
    if (old_size == ht_serial->size) {
        resize_ht(ht_finelock->ht_serial);
    }
    
    int rc;
    // release the locks in order
    for (int i = 0; i < ht_finelock->rwlocks_len; i++) {
        rc = pthread_rwlock_unlock(rwlocks[i]);
        if (rc != 0) {
            printf("%s: Failed to unlock.\n", __FUNCTION__);
            exit(1);
        }
    }
}

// optimistic
bool ht_optimistic_contains(ht_finelock_t *ht_finelock, int key) {
    SerialHashTable_t *ht_serial = ht_finelock->ht_serial;
    int index = key & ht_finelock->rwlocks_mask;
    pthread_rwlock_t *rwlock = ht_finelock->rwlocks[index];
    int rc;

    bool ret = false;
    bool valid = false;
    int my_mask = ht_serial->size - 1;
    if (ht_serial->table[key & my_mask] != NULL) {
        ret = contains_list(ht_serial->table[key & my_mask], key);
    }
    // validate
    while (pthread_rwlock_rdlock(rwlock)) {
        // yield if fails to acquire the read lock
        rc = sched_yield();
    }
    my_mask = ht_serial->size - 1;  // size could have changed
    if (ht_serial->table[key & my_mask] != NULL) {
        valid = contains_list(ht_serial->table[key & my_mask], key);
    }
    rc = pthread_rwlock_unlock(rwlock);
    if (rc != 0) {
        printf("%s: Failed to unlock.\n", __FUNCTION__);
        exit(1);
    }
    if (ret == true && valid == false) {
        while (pthread_rwlock_rdlock(rwlock)) {
            // yield if fails to acquire the read lock
            rc = sched_yield();
        }
        ret = contains_ht(ht_finelock->ht_serial, key);
        rc = pthread_rwlock_unlock(rwlock);
        if (rc != 0) {
            printf("%s: Failed to unlock.\n", __FUNCTION__);
            exit(1);
        }
    }
    return ret;
}