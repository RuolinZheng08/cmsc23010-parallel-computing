#include "ht_utils.h"

ht_utils_t *ht_utils_init(char *ht_type, int log_size, int max_bkt_size) {
    ht_utils_t *ht_utils = calloc(1, sizeof(ht_utils_t));

    if (ht_type == NULL) {

        SerialHashTable_t *ht_serial = createSerialHashTable(log_size, max_bkt_size);
        ht_utils->hashtable = ht_serial;
        ht_utils->ht_add = (void (*)(void *, int, volatile Packet_t *pkt)) &add_ht;
        ht_utils->ht_remove = (bool (*)(void *, int)) &remove_ht;
        ht_utils->ht_contains = (bool (*)(void *, int)) &contains_ht;

    } else if (strcmp(ht_type, "finelock") == 0) {

        ht_finelock_t *ht_finelock = ht_finelock_init(log_size, max_bkt_size);
        ht_utils->hashtable = ht_finelock;
        ht_utils->ht_add = (void (*)(void *, int, volatile Packet_t *pkt)) &ht_finelock_add;
        ht_utils->ht_remove = (bool (*)(void *, int)) &ht_finelock_remove;
        ht_utils->ht_contains = (bool (*)(void *, int)) &ht_finelock_contains;

    } else if (strcmp(ht_type, "optimistic") == 0) {

        ht_finelock_t *ht_finelock = ht_finelock_init(log_size, max_bkt_size);
        ht_utils->hashtable = ht_finelock;
        ht_utils->ht_add = (void (*)(void *, int, volatile Packet_t *pkt)) &ht_finelock_add;
        ht_utils->ht_remove = (bool (*)(void *, int)) &ht_finelock_remove;
        // use optimistic contains()
        ht_utils->ht_contains = (bool (*)(void *, int)) &ht_optimistic_contains;

    } else {
        printf("Invalid hashtable type\n");
        exit(1);
    }

    return ht_utils;
}

bool ht_utils_apply_operation(ht_utils_t *ht_utils, HashPacket_t *pkt) {
    bool rc = true;
    switch (pkt->type) {
        case Add:
            ht_utils->ht_add(ht_utils->hashtable, pkt->key, pkt->body);
            break;
        case Remove:
            rc = ht_utils->ht_remove(ht_utils->hashtable, pkt->key);
            break;
        case Contains:
            rc = ht_utils->ht_contains(ht_utils->hashtable, pkt->key);
            break;
    }
    return rc;
}