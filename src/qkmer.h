#ifndef QKMER_H
#define QKMER_H

#include "postgres.h"

#define QKMER_MAX_LENGTH 32

typedef struct QKmer
{
    int32 vl_len_;
    char data[FLEXIBLE_ARRAY_MEMBER]; // stored as uppercase IUPAC chars 
} QKmer;

#endif // QKMER_H
