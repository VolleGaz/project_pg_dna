#ifndef PG_DNA_H
#define PG_DNA_H

#include "postgres.h"
#include "fmgr.h"

/*
 * Internal representation of the 'dna' type
 * -----------------------------------------
 * This is a varlena type: the first field is 'vl_len_', managed by PostgreSQL.
 * After that, we store a 32-bit integer 'length' representing the number of
 * logical bases (A/C/G/T), followed by a flexible array 'data' containing
 * the packed bases encoded using 2 bits per base (4 bases per byte).
 */

typedef struct
{
    int32   vl_len_;   /* Total size of the value (PostgreSQL varlena header) */
    uint32  length;    /* Logical number of bases in the sequence */
    unsigned char data[FLEXIBLE_ARRAY_MEMBER]; /* Packed bases (2 bits per base) */
} Dna;

/*
 * Utility macros for converting between Datum and Dna*
 */

#define DatumGetDnaP(X)         ((Dna *) PG_DETOAST_DATUM(X))
#define DatumGetDnaPCopy(X)     ((Dna *) PG_DETOAST_DATUM_COPY(X))
#define DnaPGetDatum(X)         PointerGetDatum(X)

/*
 * Convenience macros
 */

#define DNA_LENGTH(dna)         ((dna)->length)
#define DNA_DATA(dna)           ((dna)->data)

/*
 * 2-bit encoding of DNA bases
 */

#define DNA_BASE_A   0x0   /* 00 */
#define DNA_BASE_C   0x1   /* 01 */
#define DNA_BASE_G   0x2   /* 10 */
#define DNA_BASE_T   0x3   /* 11 */

/*
 * Number of packed bytes required to store n bases
 * (4 bases per byte, 2 bits per base)
 */
#define DNA_PACKED_BYTES(n)     (((n) + 3) / 4)

#endif /* PG_DNA_H */
