#ifndef PG_DNA_DNA_H
#define PG_DNA_DNA_H

#include "postgres.h"

/*
 * Internal representation of the dna type (varlena).
 *
 * Layout:
 *   [ int32 vl_len_ ]   -- varlena header
 *   [ uint32 length ]   -- number of bases
 *   [ unsigned char data[] ] -- packed bases
 */
typedef struct Dna
{
    int32         vl_len_;   /* varlena header */
    uint32        length;    /* number of bases */
    unsigned char data[FLEXIBLE_ARRAY_MEMBER]; /* packed bases */
} Dna;

/* 4 bases par octet => (n + 3) / 4 */
#define DNA_PACKED_BYTES(n)   (((n) + 3) / 4)

#endif /* PG_DNA_DNA_H */
