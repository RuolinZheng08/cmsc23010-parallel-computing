#include "test_hashtable_concurrent.h"  // MACRO definitions

/* should have 6 tests for all 6 pairwise combinations of add, remove, contains */
/* 3 tests for now, add-add, rmv-rmv, contains-contains (finelock + optimistic) */
/* use MACRO definitions in .h for parameters */
int main() {
    printf("Testing concurrent add:\n");
    test_generic(getAddPacket, NULL, 0, false);  // no need to preload
    printf("Testing concurrent remove:\n");
    test_generic(getRemovePacket, NULL, num_pkts, false);
    printf("Testing concurrent contains:\n");
    test_generic(getContainsPacket, NULL, num_pkts, true);

    // contains + dummy add; remove + dummy add; remove + dummy contains
    printf("Testing concurrent contains and dummy add:\n");
    test_generic(getContainsPacket, getAddPacket, num_pkts, true);
    printf("Testing concurrent remove and dummy add:\n");
    test_generic(getRemovePacket, getAddPacket, num_pkts, false);
    printf("Testing concurrent remove and dummy contains:\n");
    test_generic(getRemovePacket, getContainsPacket, num_pkts, true);
}

/* Start Tests Body */

void test_generic(HashPacket_t * (*get_valid_packet)(HashPacketGenerator_t *source),
    HashPacket_t * (*get_dummy_packet)(HashPacketGenerator_t *source), int preload, bool optimistic) {

    HashPacketGenerator_t *source = createHashPacketGenerator(fraction_add, 
        fraction_remove, hit_rate, mean);

    ht_utils_t *serial_ht_utils = ht_utils_init(NULL, log_size, max_bkt_size);
    ht_utils_t *finelock_ht_utils = ht_utils_init("finelock", log_size, max_bkt_size);
    ht_utils_t *optim_ht_utils = NULL;
    if (optimistic) {
        optim_ht_utils = ht_utils_init("optimistic", log_size, max_bkt_size);
    }

    // preload packets here for concurrent remove() and contains()
    HashPacket_t *pkt;
    if (optimistic) {
        for (int i = 0; i < preload; i++) {
            pkt = getAddPacket(source);
            ht_utils_apply_operation(serial_ht_utils, pkt);
            ht_utils_apply_operation(finelock_ht_utils, pkt);
            ht_utils_apply_operation(optim_ht_utils, pkt);
        }
    } else {
        for (int i = 0; i < preload; i++) {
            pkt = getAddPacket(source);
            ht_utils_apply_operation(serial_ht_utils, pkt);
            ht_utils_apply_operation(finelock_ht_utils, pkt);
        }
    }
    printf("Preloaded %d packets.\n", preload);

    // serial and parallel share the packet array but have their unique hashtable
    // get_packet2 will get dummy packets
    HashPacket_t **pkts = gen_packets_arr(source, get_valid_packet, get_dummy_packet);

    long serial_res = serial_generic(serial_ht_utils, pkts);
    long finelock_res = parallel_generic(finelock_ht_utils, pkts);
    if (optimistic) {
        long optim_res = parallel_generic(optim_ht_utils, pkts);
        printf("serial: %ld; finelock: %ld; optimistic: %ld\n\n", 
            serial_res, finelock_res, optim_res);
        assert((serial_res == finelock_res) && (serial_res == optim_res));
    } else {
        printf("serial: %ld; finelock: %ld\n\n", serial_res, finelock_res);
        assert(serial_res == finelock_res);
    }

    // clean up
    for (int i = 0; i < num_pkts; i++) {
        free(pkts[i]);
    }
    free(pkts);
}

/* End Tests Body */

/* Start Packet Helpers */

HashPacket_t **gen_packets_arr(HashPacketGenerator_t *source, 
    HashPacket_t * (*get_valid_packet)(HashPacketGenerator_t *source),
    HashPacket_t * (*get_dummy_packet)(HashPacketGenerator_t *source)) {

    printf("Generating packets...\n");
    HashPacket_t **pkts;
    if (get_dummy_packet == NULL) {
        pkts = calloc(num_pkts, sizeof(HashPacket_t *));
        for (int i = 0; i < num_pkts; i += 1) {
            pkts[i] = get_valid_packet(source);
        }
    } else {
        // contains + dummy add; remove + dummy add; remove + dummy contains
        if (get_valid_packet == getAddPacket || get_dummy_packet == getRemovePacket) {
            printf("Invalid get_packet functions.\n");
            exit(1);
        } else {
            pkts = gen_valid_and_dummy(source, get_valid_packet, get_dummy_packet);  
        }
    }
    printf("Finished generating packets.\n");
    return pkts;
}

bool key_clash(HashPacket_t **pkts, HashPacket_t *this_pkt) {
    for (int i = 0; i < num_pkts / 2; i++) {
        if (this_pkt->key == pkts[i]->key) {
            printf("Packet key clash. Regenerating...\n");
            return true;
        }
    }
    return false;
}

HashPacket_t **gen_valid_and_dummy(HashPacketGenerator_t *source, 
    HashPacket_t * (*get_valid_packet)(HashPacketGenerator_t *source),
    HashPacket_t * (*get_dummy_packet)(HashPacketGenerator_t *source)) {

    HashPacket_t **pkts = calloc(num_pkts, sizeof(HashPacket_t *));
    // valid packets
    for (int i = 0; i < num_pkts / 2; i++) {
        pkts[i] = get_valid_packet(source);
    }
    // fill the rest of the packet array with dummy packets
    HashPacket_t *dummy_pkt;
    for (int i = num_pkts / 2; i < num_pkts; i++) {
        dummy_pkt = get_dummy_packet(source);
        // loop until getting a packet whose key is not used by remove()
        // still possible that these packets hash to the same bucket list
        while (key_clash(pkts, dummy_pkt)) {
            dummy_pkt = get_dummy_packet(source);
        }
        pkts[i] = dummy_pkt;
    }
    return pkts;
}

/* End Packet Helpers */

/* Start Tests Helpers */

long serial_generic(ht_utils_t *ht_utils, HashPacket_t **pkts) {
    bool rc;
    long finger_total = 0;
    long finger;
    for (int i = 0; i < num_pkts; i++) {
        rc = ht_utils_apply_operation(ht_utils, pkts[i]);
        finger = getFingerprint(pkts[i]->body->iterations, pkts[i]->body->seed); 
        if (rc == true) {
            finger_total += finger;
        }
    }
    return finger_total;
}

void *worker_generic(void *args) {
    testtargs_t *targs = args;
    HashPacket_t **pkts = targs->pkts;
    ht_utils_t *ht_utils = targs->ht_utils;
    bool rc;
    long finger;
    long finger_total = 0;

    for (int i = targs->chunk * targs->tid; i < targs->chunk * (targs->tid + 1); i++) {
        rc = ht_utils_apply_operation(ht_utils, pkts[i]);
        finger = getFingerprint(pkts[i]->body->iterations, pkts[i]->body->seed); 
        if (rc == true) {
            finger_total += finger;
        }
    }
    pthread_exit((void *) finger_total);  // assume all operation succeed, ret should be 10
}

long parallel_generic(ht_utils_t *ht_utils, HashPacket_t **pkts) {
    pthread_t workers[num_workers];
    testtargs_t targs_arr[num_workers];
    int rc;
    for (int t = 0; t < num_workers; t++) {
        targs_arr[t].tid = t;
        targs_arr[t].chunk = chunk_size;
        targs_arr[t].pkts = pkts;
        targs_arr[t].ht_utils = ht_utils;
        rc = pthread_create(&workers[t], NULL, worker_generic, (void *) &targs_arr[t]);
    }

    long local_ret;
    long global_ret = 0;
    for (int t = 0; t < num_workers; t++) {
        pthread_join(workers[t], (void *) &local_ret);
        global_ret += local_ret;
    }
    rc--;  // compiler
    return global_ret;
}

/* End Tests Helpers */