#ifndef KMER_H
#define KMER_H

#include "postgres.h"

/* Max allowed kmer length */
#define KMER_MAX_LENGTH 32

typedef struct Kmer
{
    int32 vl_len_;
    unsigned char data[FLEXIBLE_ARRAY_MEMBER];
} Kmer;

#endif /* KMER_H */
