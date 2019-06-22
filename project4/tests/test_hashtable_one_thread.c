#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <criterion/criterion.h>
#include "hashgenerator.h"
#include "ht_utils.h"

#define fraction_add 0.45
#define fraction_remove 0.05
#define hit_rate 0.9
#define mean 1

/* Start Serial Tests */

void add_remove_contains(char *ht_type) {
    HashPacketGenerator_t *source = createHashPacketGenerator(fraction_add, 
        fraction_remove, hit_rate, mean);
    int log_size = 2;  // num_workers buckets in table
    int max_bkt_size = 2;  // 2 items at most per bucket
    ht_utils_t *ht_utils = ht_utils_init(ht_type, log_size, max_bkt_size);

    bool rc;
    HashPacket_t *pkt = NULL;
    pkt = getAddPacket(source);
    ht_utils->ht_add(ht_utils->hashtable, pkt->key, pkt->body);
    rc = ht_utils->ht_contains(ht_utils->hashtable, pkt->key);
    cr_expect(rc == true);
    rc = ht_utils->ht_remove(ht_utils->hashtable, pkt->key);
    cr_expect(rc == true);
    rc = ht_utils->ht_contains(ht_utils->hashtable, pkt->key);
    cr_expect(rc == false);
    rc = ht_utils->ht_remove(ht_utils->hashtable, 999);
    cr_expect(rc == false);
}

Test(serial, add_remove_contains) {
    add_remove_contains(NULL);
}

Test(finelock, add_remove_contains) {
    add_remove_contains("finelock");
}

Test(optimistic, add_remove_contains) {
    add_remove_contains("optimistic");
}

void resize_add_contains(char *ht_type) {
    HashPacketGenerator_t *source = createHashPacketGenerator(fraction_add, 
        fraction_remove, hit_rate, mean);
    int log_size = 2;  // num_workers buckets in table
    int max_bkt_size = 2;  // 2 items at most per bucket
    ht_utils_t *ht_utils = ht_utils_init(ht_type, log_size, max_bkt_size);

    bool rc;
    int num = 16;
    HashPacket_t **pkts = calloc(num, sizeof(HashPacket_t *));

    for (int i = 0; i < num; i++) {
        pkts[i] = getAddPacket(source);
        ht_utils->ht_add(ht_utils->hashtable, pkts[i]->key, pkts[i]->body);
    }
    for (int i = 0; i < num; i++) {
        rc = ht_utils->ht_contains(ht_utils->hashtable, pkts[i]->key);
        cr_expect(rc == true);
    }
}

Test(serial, resize_add_contains) {
    resize_add_contains(NULL);
}

Test(finelock, resize_add_contains) {
    resize_add_contains("finelock");
}

Test(optimistic, resize_add_contains) {
    resize_add_contains("optimistic");
}

/* End Serial Tests */