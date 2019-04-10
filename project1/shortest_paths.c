#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// Phase 1: Read input and initialize dist array
int **parse_dist(FILE *fptr, int N) {
    char line[256];
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

// Phase 2 Serial: Directly modifies dist
void shortest_paths_serial(int **dist, int N) {
    printf("Serial\n");
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

// Phase 2 Parallel: Directly modifies dist
void shortest_paths_parallel(int **dist, int N) {
    printf("Parallel\n");
}

// Phase 3: Write dist to file
void output(FILE *fout, int **dist, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(fout, "%-10d", dist[i][j]);
        }
        fprintf(fout, "\n");
    }
    fclose(fout);
}

int main(int argc, char **argv) {
    int opt;
    char *fname;
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
    // Phase 1 Input
    FILE *fptr = fopen(fname, "r");
    if (fptr == NULL) {
        printf("Usage: \nSerial: ./shortest_paths -f FILE\n"
            "Parallel: ./shortest_paths -f FILE -t NUM_THREADS\n");
        exit(1);
    }
    char line[256];
    fgets(line, sizeof(line), fptr);
    int N = atoi(strtok(line, "\n"));
    printf("N: %d\n", N);
    int **dist = parse_dist(fptr, N);

    // Phase 2 Floyd-Warshall Algorithm
    if (T == 0) {
        shortest_paths_serial(dist, N);
    } else if (T > 0) {
        shortest_paths_parallel(dist, N);
    }

    // Phase 3 Output
    char *foutname = calloc(7 + strlen(fname), sizeof(char));
    strcpy(foutname, "result_");
    strcat(foutname, fname);
    printf("Output: %s\n", foutname);
    FILE *fout = fopen(foutname, "w");
    if (fout == NULL) {
        printf("Error writing to file.\n");
        exit(1);
    }
    output(fout, dist, N);
}