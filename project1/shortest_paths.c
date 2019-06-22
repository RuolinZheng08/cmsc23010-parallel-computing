#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "../utils/stopwatch.h"

// thread argument for Barrier version
typedef struct {
    int t;
    int N;
    int chunk;
    int **dist;
} btargs_t;

// thread argument
typedef struct {
    int t;
    int k;
    int N;
    int chunk;
    int **dist;
} targs_t;

pthread_barrier_t barrier;

// Phase 1: read input and initialize dist array
int **parse_dist(FILE *fptr, int N) {
    char line[20480];
    char *token = NULL;
    int i, j;
    int **dist = calloc(N, sizeof(*dist));
    for (i = 0; i < N; i++) {
        dist[i] = calloc(N, sizeof(int));
    }

    i = 0;
    while (fgets(line, sizeof(line), fptr)) {
        j = 0;
        token = strtok(line, " \n");
        while (token != NULL) {
            dist[i][j] = atoi(token);
            token = strtok(NULL, " \n");
            j++;
        }
        i++;
    }
    fclose(fptr);
    return dist;
}

// Phase 2 Serial: directly modifies dist
void shortest_paths_serial(int **dist, int N) {
    for (int k = 0; k < N; k++) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (dist[i][j] > dist[i][k] + dist[k][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                }
            }
        }
    }
}

/* Barrier Static */
// Phase 2 Parallel: worker function that waits on barrier
void *worker_func(void *btargs) {
    btargs_t *btargs_new = btargs;
    int t = btargs_new->t;
    int N = btargs_new->N;
    int **dist = btargs_new->dist;
    int start = t * btargs_new->chunk;
    int end = start + btargs_new->chunk;
    for (int k = 0; k < N; k++) {
        for (int i = start; i < end; i++) {
            for (int j = 0; j < N; j++) {
                if (dist[i][j] > dist[i][k] + dist[k][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                }
            }
        }
        pthread_barrier_wait(&barrier);
    }
    return NULL;
}

// Phase 2 Parallel: synchronize increments to k among threads
void shortest_paths_parallel_barrier(int **dist, int N, int T) {
    pthread_t threads[T];
    int rc;
    btargs_t btargs_arr[T];
    pthread_barrier_init(&barrier, 0, T);

    for (int t = 0; t < T; t++) {
        btargs_arr[t].t = t;
        btargs_arr[t].N = N;
        btargs_arr[t].chunk = N / T;
        btargs_arr[t].dist = dist;
        rc = pthread_create(&threads[t], 0, worker_func, 
            (void*) &btargs_arr[t]);
        if (rc != 0) {
            printf("Error creating thread.");
        }
    }
    for (int t = 0; t < T; t++) {
        pthread_join(threads[t], 0);
    }
}
/* End Barrier Static */

/* Non-barrier Static */
// Phase 2 Parallel: worker function for i, j inner loops
void *inner_loops(void *targs) {
    targs_t *targs_new = targs;
    int t = targs_new->t;
    int k = targs_new->k;
    int N = targs_new->N;
    int chunk = targs_new->chunk;
    int **dist = targs_new->dist;
    for (int i = t * chunk; i < (t + 1) * chunk; i++) {
        for (int j = 0; j < N; j++) {
            if (dist[i][j] > dist[i][k] + dist[k][j]) {
                dist[i][j] = dist[i][k] + dist[k][j];
            }
        }
    }
    pthread_exit(0);
}

// Phase 2 Parallel: assign each thread a row of dist
void shortest_paths_parallel(int **dist, int N, int T) {
    pthread_t threads[T];
    int rc;
    int t;
    targs_t targs_arr[T];

    for (int k = 0; k < N; k++) {
        for (t = 0; t < T; t++) {
            targs_arr[t].t = t;
            targs_arr[t].k = k;
            targs_arr[t].N = N;
            targs_arr[t].chunk = N / T;
            targs_arr[t].dist = dist;
            rc = pthread_create(&threads[t], 0, inner_loops, 
                (void*) &targs_arr[t]);
            if (rc != 0) {
                printf("Error creating thread.");
            }
        }
        for (t = 0; t < T; t++) {
            pthread_join(threads[t], 0);
        }
    }
}
/* End Non-barrier Static */

// Phase 3: write dist to file
void output(FILE *fout, int **dist, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(fout, "%-14d", dist[i][j]);
        }
        fprintf(fout, "\n");
    }
    fclose(fout);
}

int main(int argc, char **argv) {
    int opt;
    char *fname = NULL;
    int T = 0;
    while ((opt = getopt(argc, argv, "t:f:")) != -1) {
        switch(opt) {
            case 't':
                T = atoi(optarg);
                break;
            case 'f':
                fname = optarg;
                break;
        }
    }
    if (fname == NULL) {
        printf("Usage: \nSerial: ./shortest_paths -f FILE\n"
            "Parallel: ./shortest_paths -f FILE -t NUM_THREADS\n");
        exit(1);
    }
    // Phase 1 input
    FILE *fptr = fopen(fname, "r");
    if (fptr == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }
    char line[10];
    char *res;
    res = fgets(line, sizeof(line), fptr);
    if (res == NULL) {
        printf("Error reading file.\n");
        exit(1);
    }
    int N = atoi(strtok(line, "\n"));
    int **dist = parse_dist(fptr, N);

    // Phase 2 Floyd-Warshall Algorithm
    StopWatch_t watch;
    if (T == 0) {
        startTimer(&watch);
        shortest_paths_serial(dist, N);
    } else if (T > 0) {
        int Tnew = T > N ? N : T;
        startTimer(&watch);
        shortest_paths_parallel_barrier(dist, N, Tnew);
    } else {
        printf("Invalid number of threads.\n");
        exit(1);
    }
    stopTimer(&watch);
    printf("%0.3f", getElapsedTime(&watch));

    // Phase 3 output
    char *foutname = calloc(10 + strlen(fname), sizeof(char));
    strcpy(foutname, fname);
    foutname[strlen(fname) - 4] = '\0';
    if (T == 0) {
        strcat(foutname, "_s");
    } else {
        strcat(foutname, "_p");
    }
    strcat(foutname, "_res.txt");

    FILE *fout = fopen(foutname, "w");
    if (fout == NULL) {
        printf("Error writing to file.\n");
        exit(1);
    }
    output(fout, dist, N);
}
