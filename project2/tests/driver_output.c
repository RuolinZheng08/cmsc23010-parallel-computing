#define _GNU_SOURCE
#include "driver_output.h"
#define SEED 0

int main(int argc, char **argv) {
    int opt;
    int pkt_type = 0;
    int prog_type = 0;

    int num_src = -1;
    int num_pkt = -1;
    int depth = -1;
    int mean = -1;

    while ((opt = getopt(argc, argv, "n:t:d:w:p:r:")) != -1) {
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
        }
    }
    if (num_src == -1 || num_pkt == -1 || depth == -1 || mean == -1 || 
        pkt_type == 0 || strchr("cue", pkt_type) == NULL ||
        prog_type == 0 || strchr("spq", prog_type) == NULL) {
        printf("Usage: \n./driver_output -n NUM_SOURCE -t NUM_PACKET "
            "-d DEPTH -w MEAN -p PACKET_TYPE[c|u|e] "
            "-r PROGRAM_TYPE[s|p|q]\n"
            "\nExample: \n./driver_output -n 3 -t 5 -d 32 -w 25 -p c -r s\n");
        exit(1);
    }
    
    // a pointer to the specified packet generation function
    volatile Packet_t *(*get_packet)(PacketSource_t *, int);
    get_packet = &getConstantPacket;
    PacketSource_t *source = createPacketSource(mean, num_src, SEED);

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

    // initialize the array and run the specified program
    long **res = init_arr(num_pkt, num_src);
    switch (prog_type) {
        case 's':
            serial(res, source, get_packet, num_src, num_pkt);
            break;
        case 'p':
            parallel(res, source, get_packet, num_src, num_pkt, depth);
            break;
        case 'q':
            serial_queue(res, source, get_packet, num_src, num_pkt, depth);
            break;
    }

    // output the result array in res_[s|p|q].txt

    char *fname = calloc(10, sizeof(char));
    fname[0] = prog_type;
    strcat(fname, "_res.txt");
    int rc = output(fname, res, num_pkt, num_src);
    if (rc != 0) {
        printf("Error writing to file.\n");
        exit(1);
    }
    printf("Output: %s\n", fname);
}

void serial(long **res, PacketSource_t *source, 
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt) {
    long finger;
    volatile Packet_t *pkt; 
    for (int t = 0; t < num_pkt; t++) {
        for (int i = 0; i < num_src; i++) {
            pkt = (*get_packet)(source, i);
            finger = getFingerprint(pkt->iterations, pkt->seed);
            res[t][i] = finger;
        }
    }
}

void parallel(long **res, PacketSource_t *source, 
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt, int depth) {
    int rc;
    volatile Packet_t *pkt;
    lamport_queue_t *queue_arr[num_src];
    pthread_t threads[num_src];
    outargs_t outargs_arr[num_src];

    // create num_src queues and assign to workers
    for (int i = 0; i < num_src; i++) {
        queue_arr[i] = init_queue(depth);
        outargs_arr[i].num_pkt = num_pkt;
        outargs_arr[i].cur_src = i;
        outargs_arr[i].queue = queue_arr[i];
        outargs_arr[i].res = res;
        rc = pthread_create(&threads[i], NULL, worker_func, 
            (void *)&outargs_arr[i]);
        if (rc != 0) {
            printf("Error creating thread.");
        }
    }
    // dispatch packets
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

void serial_queue(long **res, PacketSource_t *source, 
    volatile Packet_t *(*get_packet)(PacketSource_t *, int),
    int num_src, int num_pkt, int depth) {
    long finger;
    volatile Packet_t *pkt; 
    volatile Packet_t *deq_pkt;
    lamport_queue_t *queue = init_queue(depth);

    for (int t = 0; t < num_pkt; t++) {
        for (int i = 0; i < num_src; i++) {
            pkt = (*get_packet)(source, i);
            while (enqueue(queue, (void *) pkt)) {
                // spin if queue is full
            }
            deq_pkt = (Packet_t *) dequeue(queue);
            finger = getFingerprint(deq_pkt->iterations, deq_pkt->seed);
            res[t][i] = finger;
        }
    }
}

void *worker_func(void *outargs) {
    int counter = 0;
    long finger;
    int rc = 0;
    rc++;  // turn off compiler warning
    outargs_t *outargs_new = (outargs_t *) outargs;
    int cur_src = outargs_new->cur_src;
    long **res = outargs_new->res;
    volatile Packet_t *pkt = NULL;
    while (counter < outargs_new->num_pkt) {
        pkt = (Packet_t *) dequeue(outargs_new->queue);
        if (pkt != NULL) {
            finger = getFingerprint(pkt->iterations, pkt->seed);
            res[counter][cur_src] = finger;
            counter++;
        } else {
            rc = pthread_yield();
        }
    }
    return NULL;
}

// initialize an empty num_pkt * num_src array to store fingerprints
long **init_arr(int num_row, int num_col) {
    long **res = calloc(num_row, sizeof(*res));
    for (int i = 0; i < num_row; i++) {
        res[i] = calloc(num_col, sizeof(long));
    }
    return res;
}

// write to file given filename
int output(char *fname, long **res, int num_row, int num_col) {
    FILE *fout = fopen(fname, "w");
    if (fout == NULL) {
        return 1;
    }
    for (int i = 0; i < num_row; i++) {
        for (int j = 0; j < num_col; j++) {
            fprintf(fout, "%-14ld", res[i][j]);
        }
        fprintf(fout, "\n");
    }
    fclose(fout);
    return 0;
}