
#include<stdio.h>
#include<stdlib.h>
#include "statistics.h"
#include <math.h>

double getStdDev2(long count[], int size) {
	double sum = 0;
	double sumSquared = 0;
	for(int i = 0; i < size; i++) {
	sum += (double) count[i];
	sumSquared += (double) (count[i]*count[i]);
	}

	double mean = (sum/ ((double) size));
	double variance = sumSquared/((double) size) - mean*mean;

	return sqrt(variance);
}

double getEntropy(long count[], int size){
    double * p = (double *)malloc(sizeof(double) * size);
    double total = 0;
    double entropy = 0;
    for(int i = 0; i < size; i++) {
      p[i] = (double) count[i];
      total += p[i];
    }
    for(int i = 0; i < size; i++) {
      p[i] /= total;
      entropy += p[i]*log(p[i])/log(2.0);
    }
    free(p);
    return -entropy;

}
