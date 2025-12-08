#include "postgres.h"
#if PG_VERSION_NUM >= 160000
#include "varatt.h"
#endif
#include "fmgr.h"
#include "utils/varlena.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "qkmer.h"

#include <ctype.h>
#include <string.h>

/*
 * qkmer: query k-mer with ambiguity codes (IUPAC)
 *
 * Stored as a varlena struct:
 *
 *   struct QKmer {
 *       int32 vl_len_;
 *       char  data[FLEXIBLE_ARRAY_MEMBER];   -- n characters, no '\0'
 *   }
 *
 * Allowed characters (case-insensitive on input, stored as uppercase):
 *   A C G T
 *   N R Y S W K M B D H V
 */


// Normalize and validate a qkmer base; return uppercase IUPAC char 
static inline char
normalize_qbase(char c)
{
    char u = (char)toupper((unsigned char)c);

    switch (u)
    {
    case 'A':
    case 'C':
    case 'G':
    case 'T':
    case 'N':
    case 'R':
    case 'Y':
    case 'S':
    case 'W':
    case 'K':
    case 'M':
    case 'B':
    case 'D':
    case 'H':
    case 'V':
        return u;

    default:
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("invalid qkmer base: '%c' (allowed: A,C,G,T,N,R,Y,S,W,K,M,B,D,H,V)", c)));
        return 'A';
    }
}

// Compute the number of characters stored in this qkmer 
static inline int
qkmer_length_internal(const QKmer *q)
{
    Size size = VARSIZE_ANY(q);
    Size header = offsetof(QKmer, data);

    if (size < header)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("qkmer value is corrupted")));

    return (int)(size - header);
}

// Sanity checks: length and size consistency 
static void
check_qkmer_consistency(const QKmer *q)
{
    int n;
    Size expected;

    n = qkmer_length_internal(q);

    if (n < 0 || n > QKMER_MAX_LENGTH)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("qkmer value has unreasonable length")));

    expected = offsetof(QKmer, data) + n;
    if (VARSIZE_ANY(q) != expected)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("qkmer value has invalid internal size")));
}


//Input function: cstring -> qkmer

PG_FUNCTION_INFO_V1(qkmer_in);

Datum qkmer_in(PG_FUNCTION_ARGS)
{
    char *input;
    int n;
    Size size;
    QKmer *q;

    input = PG_GETARG_CSTRING(0);
    n = (int)strlen(input);

    if (n == 0)
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("qkmer cannot be empty")));

    if (n > QKMER_MAX_LENGTH)
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("qkmer length %d exceeds maximum %d", n, QKMER_MAX_LENGTH)));

    size = offsetof(QKmer, data) + n;

    if (size > MaxAllocSize)
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("qkmer is too large")));

    q = (QKmer *)palloc(size);
    SET_VARSIZE(q, size);

    for (int i = 0; i < n; i++)
        q->data[i] = normalize_qbase(input[i]);

    PG_RETURN_POINTER(q);
}


//Output function: qkmer -> cstring

PG_FUNCTION_INFO_V1(qkmer_out);

Datum qkmer_out(PG_FUNCTION_ARGS)
{
    QKmer *q;
    int n;
    char *res;

    q = (QKmer *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    check_qkmer_consistency(q);

    n = qkmer_length_internal(q);

    res = (char *)palloc(n + 1);
    memcpy(res, q->data, n);
    res[n] = '\0';

    PG_RETURN_CSTRING(res);
}


PG_FUNCTION_INFO_V1(qkmer_length);

Datum qkmer_length(PG_FUNCTION_ARGS)
{
    QKmer *q;
    int n;

    q = (QKmer *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

    check_qkmer_consistency(q);

    n = qkmer_length_internal(q);

    PG_RETURN_INT32(n);
}
