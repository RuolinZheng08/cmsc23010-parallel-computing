/*
 * fingerprint.c
 *
 *  Created on: Apr 7, 2013
 *      Author: lamont
 */

#include "fingerprint.h"

const long m = (long) 0xFFFFFFFFFFFFL;
const long a = 25214903917L;
const long c = 11L;


long getFingerprint(long iterations, long startSeed) {
    long seed = startSeed;
    for(long i = 0; i < iterations; i++) {
      seed = (seed*a + c) & m;
    }
    return ( seed >> 12 );
}
