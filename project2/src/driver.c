#define _GNU_SOURCE
#include "driver.h"

StopWatch_t watch;

int main(int argc, char **argv) {
    int opt;
    int pkt_type = 0;
    int prog_type = 0;

    int num_src = -1;
    int num_pkt = -1;
    int depth = -1;
    int mean = -1;

    int randseed = 0;  // default

    while ((opt = getopt(argc, argv, "n:t:d:w:p:r:s:")) != -1) {
        switch(opt) {
            case 'n':
                num_src = atoi(optarg) - 1;
                break;
            case 't':
                num_pkt = atoi(optarg);
                break;
            case 'd':
                depth = atoi(optarg);
                break;
            case 'w':
                mean = atoi(optarg);
                break;
            case 'p':  // constant 'c', uniform 'u', or exponential 'e'
                pkt_type = optarg[0];
                break;
            case 'r':  // serial 's', serial-queue 'q', parallel 'p'
                prog_type = optarg[0];
                break;
            case 's':
                randseed = atoi(optarg);
                break;
        }
    }
    if (num_src == -1 || num_pkt == -1 || depth == -1 || mean == -1 || 
        pkt_type == 0 || strchr("cue", pkt_type) == NULL ||
        prog_type == 0 || strchr("spq", prog_type) == NULL) {
        printf("Usage: \n./driver -n NUM_SOURCE -t NUM_PACKET "
            "-d DEPTH -w MEAN -p PACKET_TYPE[c|u|e] "
            "-r PROGRAM_TYPE[s|p|q] -s SEED\n"
            "\nExample: \n./driver -n 3 -t 5 -d 32 -w 25 -p c -r s\n");
        exit(1);
    }
    
    // a pointer to the specified packet generation function
    volatile Packet_t *(*get_packet)(PacketSource_t *, int);
    get_packet = &getConstantPacket;
    PacketSource_t *source = createPacketSource(mean, num_src, randseed);

    switch (pkt_type) {
        case 'c':
            get_packet = &getConstantPacket;
            break;
        case 'u':
            get_packet = &getUniformPacket;
            break;
        case 'e':
            get_packet = &getExponentialPacket;
            break;
    }

    // run the specified program
    switch (prog_type) {
        case 's':
            printf("%-14s", "Serial");
            serial(source, get_packet, num_src, num_pkt);
            break;
        case 'p':
            printf("%-14s", "Parallel");
            parallel(source, get_packet, num_src, num_pkt, depth);
            break;
        case 'q':
            printf("%-14s", "Serial-queue");
            serial_queue(source, get_packet, num_src, num_pkt, depth);
            break;
    }
    stopTimer(&watch);
    printf("time: %0.3f\n", getElapsedTime(&watch));
}

void serial(PacketSource_t *source, 
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt) {
    long finger = 0;
    finger++;  // turn off compiler warning
    volatile Packet_t *pkt;
    startTimer(&watch);
    for (int t = 0; t < num_pkt; t++) {
        for (int i = 0; i < num_src; i++) {
            pkt = (*get_packet)(source, i);
            finger = getFingerprint(pkt->iterations, pkt->seed);
        }
    }
}

void parallel(PacketSource_t *source, 
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt, int depth) {
    int rc;
    volatile Packet_t *pkt;
    lamport_queue_t *queue_arr[num_src];
    pthread_t threads[num_src];
    targs_t targs_arr[num_src];

    // create num_src queues and assign to workers
    for (int i = 0; i < num_src; i++) {
        queue_arr[i] = init_queue(depth);
        targs_arr[i].num_pkt = num_pkt;
        targs_arr[i].queue = queue_arr[i];
        rc = pthread_create(&threads[i], NULL, worker_func, 
            (void *)&targs_arr[i]);
        if (rc != 0) {
            printf("Error creating thread.\n");
        }
    }

    // dispatch packets
    startTimer(&watch);
    for (int t = 0; t < num_pkt; t++) {
        for (int i = 0; i < num_src; i++) {
            pkt = (*get_packet)(source, i);
            while (enqueue(queue_arr[i], (void *) pkt)) {
                // spin if queue is full
            }
        }
    }
    // join threads
    for (int i = 0; i < num_src; i++) {
        pthread_join(threads[i], NULL);
    }
}

void *worker_func(void *targs) {
    int counter = 0;
    long finger = 0;
    int rc = 0;
    finger++;  // turn off compiler warning
    rc++;
    targs_t *targs_new = (targs_t *) targs;
    volatile Packet_t *pkt = NULL;
    while (counter < targs_new->num_pkt) {
        pkt = (Packet_t *) dequeue(targs_new->queue);
        if (pkt != NULL) {
            finger = getFingerprint(pkt->iterations, pkt->seed);
            counter++;
        } else {
            rc = pthread_yield();
        }
    }
    return NULL;
}

void serial_queue(PacketSource_t *source, 
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt, int depth) {
    long finger = 0;
    finger++;  // turn off compiler warning
    volatile Packet_t *pkt; 
    volatile Packet_t *deq_pkt;
    lamport_queue_t *queue = init_queue(depth);

    startTimer(&watch);
    for (int t = 0; t < num_pkt; t++) {
        for (int i = 0; i < num_src; i++) {
            pkt = (*get_packet)(source, i);
            while (enqueue(queue, (void *) pkt)) {
                // spin if queue is full
            }
            deq_pkt = (Packet_t *) dequeue(queue);
            finger = getFingerprint(deq_pkt->iterations, deq_pkt->seed);
        }
    }
}
