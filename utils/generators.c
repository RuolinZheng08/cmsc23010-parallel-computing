
#include "generators.h"
#include <math.h>
const char * randomString = "This is going to be my randomization string";
const unsigned int randomStringSize = 43;

long initGenerator(RandomGenerator_t * gen) {
	gen->crc = crc32(0, randomString,randomStringSize);
	gen->seed = 1;
	return (long)gen->crc;
}
long updateRand(RandomGenerator_t * gen) {
	char buffer[1];
	buffer[0] = (char)gen->seed;
	gen->crc = crc32(gen->crc, buffer,1);
	return (long)gen->crc;
}
long getUniformRand(UniformGenerator_t * uGen) {
	updateRand(&(uGen->randGen));
	return ((long)uGen->randGen.crc % uGen->maxValue);
}
long genExponentialRand(ExponentialGenerator_t * eGen){
	updateRand(&(eGen->randGen));
    double tmpCDF = (double) (eGen->randGen.crc & 0xFFFFFFFF);
    tmpCDF = tmpCDF / 4294967295.0; //0xFFFFFFFF in decimal
    return (long)ceil(-eGen->mean*log(1.0-tmpCDF));
}


