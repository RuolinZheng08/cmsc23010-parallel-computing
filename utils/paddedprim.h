#ifndef PADDEDPRIM_H_
#define PADDEDPRIM_H_

#include <stdbool.h>

typedef struct
{
	long pad[8];
	volatile long value;
	long pad2[8];
} PaddedPrimLong_t;

typedef struct
{
	long pad[8];
	long value;
	long pad2[8];
} PaddedPrimLong_NonVolatile_t;


typedef struct
{
	long pad[8];
	volatile bool value;
	long pad2[8];
} PaddedPrimBool_t;

typedef struct
{
	long pad[8];
	bool value;
	long pad2[8];
} PaddedPrimBool_NonVolatile_t;


#endif /* PADDEDPRIM_H_ */
