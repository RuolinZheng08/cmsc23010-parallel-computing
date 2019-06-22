#ifndef GENERATORS_H_
#define GENERATORS_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "crc32.h"


typedef struct {
	long seed;
	long crc;
}RandomGenerator_t;

typedef struct {
	long maxValue;
	RandomGenerator_t randGen;
}UniformGenerator_t;

typedef struct {
	double mean;
	RandomGenerator_t randGen;
}ExponentialGenerator_t;


long initGenerator(RandomGenerator_t * gen);
long updateRand(RandomGenerator_t * gen);
long getUniformRand(UniformGenerator_t * uGen);
long genExponentialRand(ExponentialGenerator_t * eGen);

#endif /* GENERATORS_H_ */
