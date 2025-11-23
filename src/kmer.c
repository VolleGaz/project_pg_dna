#include "postgres.h"
#include "varatt.h"
#include "fmgr.h"
#include "utils/varlena.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "kmer.h"

#include <ctype.h>
#include <string.h>

/* Max length for k-mers */
#define KMER_MAX_LENGTH 32

/* Number of packed bytes needed for n characters (4 bases per byte) */
#define KMER_PACKED_BYTES(n) (((n) + 3) / 4)

/* ------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------
 */

/* Encode A/C/G/T into 0/1/2/3 */
static inline unsigned char
encode_base(char c)
{
    switch (toupper((unsigned char)c))
    {
    case 'A':
        return 0;
    case 'C':
        return 1;
    case 'G':
        return 2;
    case 'T':
        return 3;
    default:
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("invalid kmer base: '%c' (allowed: A,C,G,T only)", c)));
        return 0; /* unreachable */
    }
}

/* Decode 0/1/2/3 back to A/C/G/T */
static inline char
decode_base(unsigned char x)
{
    static const char table[4] = {'A', 'C', 'G', 'T'};
    return table[x & 3];
}

/* Compute the number of bases stored in this kmer value */
static inline int
kmer_length_internal(const Kmer *k)
{
    Size size = VARSIZE_ANY(k);
    Size header = offsetof(Kmer, data);

    if (size < header)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("kmer value is corrupted")));

    return (int)((size - header) * 4);
}

/* Ensure internal consistency (length <= 32, size matches packed length) */
static void
check_kmer_consistency(const Kmer *k)
{
    int n = kmer_length_internal(k);

    if (n < 0 || n > KMER_MAX_LENGTH)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("kmer value has unreasonable length")));

    Size expected = offsetof(Kmer, data) + KMER_PACKED_BYTES(n);
    if (VARSIZE_ANY(k) != expected)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("kmer value has invalid internal size")));
}

/* ------------------------------------------------------------------------
 * Input function: cstring → kmer
 * ------------------------------------------------------------------------
 */
PG_FUNCTION_INFO_V1(kmer_in);

Datum kmer_in(PG_FUNCTION_ARGS)
{
    char *input = PG_GETARG_CSTRING(0);
    int n = (int)strlen(input);

    if (n == 0)
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("kmer cannot be empty")));

    if (n > KMER_MAX_LENGTH)
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("kmer length %d exceeds maximum %d", n, KMER_MAX_LENGTH)));

    Size data_bytes = KMER_PACKED_BYTES(n);
    Size size = offsetof(Kmer, data) + data_bytes;

    if (size > MaxAllocSize)
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("kmer is too large")));

    Kmer *k = (Kmer *)palloc0(size);
    SET_VARSIZE(k, size);

    /* Pack 4 bases per byte */
    for (int i = 0; i < n; i++)
    {
        unsigned char v = encode_base(input[i]);
        int byte_index = i / 4;
        int shift = (3 - (i % 4)) * 2;
        k->data[byte_index] |= (v << shift);
    }

    PG_RETURN_POINTER(k);
}

/* ------------------------------------------------------------------------
 * Output function: kmer → cstring
 * ------------------------------------------------------------------------
 */
PG_FUNCTION_INFO_V1(kmer_out);

Datum kmer_out(PG_FUNCTION_ARGS)
{
    Kmer *k = (Kmer *)PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(0));

    check_kmer_consistency(k);

    int n = kmer_length_internal(k);

    char *res = (char *)palloc(n + 1);

    for (int i = 0; i < n; i++)
    {
        int byte_index = i / 4;
        int shift = (3 - (i % 4)) * 2;
        unsigned char v = (k->data[byte_index] >> shift) & 3;
        res[i] = decode_base(v);
    }
    res[n] = '\0';

    PG_RETURN_CSTRING(res);
}

/* ------------------------------------------------------------------------
 * length(kmer)
 * ------------------------------------------------------------------------
 */
PG_FUNCTION_INFO_V1(kmer_length);

Datum kmer_length(PG_FUNCTION_ARGS)
{
    Kmer *k = (Kmer *)PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(0));

    check_kmer_consistency(k);

    int n = kmer_length_internal(k);

    PG_RETURN_INT32(n);
}
