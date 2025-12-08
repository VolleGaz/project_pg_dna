#ifndef PG_DNA_DNA_H
#define PG_DNA_DNA_H

#include "postgres.h"

/*
 * Number of bytes needed to store n bases with 2 bits/base.
 * 4 bases per byte => ceil(n / 4) = (n + 3) / 4.
 */
#define DNA_PACKED_BYTES(n)   (((n) + 3) / 4)

#define DNA_MAX_LENGTH   (100000000U)  

typedef struct Dna
{
    int32         vl_len_;   // varlena header  
    uint32        length;    // number of bases 
    unsigned char data[FLEXIBLE_ARRAY_MEMBER]; // packed bases 
} Dna;

#endif 
