#ifndef KMER_H
#define KMER_H

#include "postgres.h"

/* Max allowed kmer length */
#define KMER_MAX_LENGTH 32

/* Number of packed bytes needed for n characters (4 bases per byte) */
#define KMER_PACKED_BYTES(n) (((n) + 3) / 4)

/*
 * Varlen header + logical length in bases + packed data.
 *
 *   vl_len_ : standard PostgreSQL varlena length header
 *   length  : number of bases (1..32)
 *   data[]  : packed bases, 4 per byte (2 bits each)
 */
typedef struct Kmer
{
    int32 vl_len_;
    int32 length;                  /* logical length in bases */
    unsigned char data[FLEXIBLE_ARRAY_MEMBER];
} Kmer;

#endif /* KMER_H */
