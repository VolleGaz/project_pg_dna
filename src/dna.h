#ifndef PG_DNA_DNA_H
#define PG_DNA_DNA_H

#include "postgres.h"

/*
 * Internal representation of the dna type (varlena).
 *
 * Memory layout:
 *
 *   [ int32  vl_len_ ]   -- varlena header
 *   [ uint32 length ]    -- logical number of bases
 *   [ unsigned char data[] ] -- packed bases (2 bits/base)
 *
 * This struct MUST match what we allocate in dna_in().
 */
typedef struct Dna
{
    int32         vl_len_;   /* varlena header (do not touch directly) */
    uint32        length;    /* number of bases */
    unsigned char data[FLEXIBLE_ARRAY_MEMBER]; /* packed bases */
} Dna;

/*
 * Number of bytes needed to store n bases with 2 bits/base.
 * 4 bases per byte => ceil(n / 4) = (n + 3) / 4.
 */
#define DNA_PACKED_BYTES(n)   (((n) + 3) / 4)

/*
 * Hard limit to avoid absurdly large allocations.
 * You can adjust this value depending on project constraints.
 */
#define DNA_MAX_LENGTH   (100000000U)  /* 1e8 bases */

#endif /* PG_DNA_DNA_H */
