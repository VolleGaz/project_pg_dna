#include "postgres.h"
#if PG_VERSION_NUM >= 160000
#include "varatt.h"
#endif
#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "kmer.h"

#include <ctype.h>
#include <string.h>


PG_FUNCTION_INFO_V1(kmer_in);
PG_FUNCTION_INFO_V1(kmer_out);
PG_FUNCTION_INFO_V1(kmer_length);

/* ------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------ */

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
            /* not reached */
            return 0;
    }
}

/* Decode 0/1/2/3 back to A/C/G/T */
static inline char
decode_base(unsigned char x)
{
    static const char table[4] = { 'A', 'C', 'G', 'T' };
    return table[x & 3];
}

/* Compute logical number of bases stored in this kmer */
static inline int
kmer_length_internal(const Kmer *k)
{
    return k->length;
}

/* Ensure internal consistency (length <= 32, size matches packed length) */
static void
check_kmer_consistency(const Kmer *k)
{
    Size  size;
    int   n;
    Size  header;
    Size  packed_bytes;
    Size  min_size;

    size = VARSIZE_ANY(k);
    n    = k->length;

    if (n <= 0 || n > KMER_MAX_LENGTH)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("kmer value has unreasonable length: %d", n)));

    header       = offsetof(Kmer, data);
    packed_bytes = KMER_PACKED_BYTES(n);
    min_size     = header + packed_bytes;

    if (size < min_size)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("kmer value is corrupted: size %zu too small for length %d",
                        (size_t) size, n)));
}

/* ------------------------------------------------------------------------
 * Input function: cstring → kmer
 * ------------------------------------------------------------------------
 */
Datum
kmer_in(PG_FUNCTION_ARGS)
{
    char   *input;
    int     n;
    Size    header;
    Size    packed_bytes;
    Size    size;
    Kmer   *k;

    input = PG_GETARG_CSTRING(0);
    n     = (int) strlen(input);

    if (n == 0)
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("kmer cannot be empty")));

    if (n > KMER_MAX_LENGTH)
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("kmer length %d exceeds maximum %d", n, KMER_MAX_LENGTH)));

    header       = offsetof(Kmer, data);
    packed_bytes = KMER_PACKED_BYTES(n);
    size         = header + packed_bytes;

    if (size > MaxAllocSize)
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("kmer is too large")));

    k = (Kmer *) palloc0(size);
    SET_VARSIZE(k, size);
    k->length = n;

    /* Pack 4 bases per byte */
    for (int i = 0; i < n; i++)
    {
        unsigned char v = encode_base(input[i]);
        int           byte_index = i / 4;
        int           shift      = (3 - (i % 4)) * 2;

        k->data[byte_index] |= (unsigned char) (v << shift);
    }

    PG_RETURN_POINTER(k);
}

/* ------------------------------------------------------------------------
 * Output function: kmer → cstring
 * ------------------------------------------------------------------------
 */
Datum
kmer_out(PG_FUNCTION_ARGS)
{
    Kmer *k;
    int   n;
    char *res;

    /*
     * Use PG_DETOAST_DATUM to force a 4-byte header representation.
     * Small values may be stored with a 1-byte varlena header, which would
     * misalign the length field in our struct.
     */
    k = (Kmer *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    check_kmer_consistency(k);

    n   = kmer_length_internal(k);
    res = (char *) palloc(n + 1);

    for (int i = 0; i < n; i++)
    {
        int           byte_index = i / 4;
        int           shift      = (3 - (i % 4)) * 2;
        unsigned char v         = (k->data[byte_index] >> shift) & 0x03;
        res[i] = decode_base(v);
    }
    res[n] = '\0';

    PG_RETURN_CSTRING(res);
}

/* ------------------------------------------------------------------------
 * length(kmer)
 * ------------------------------------------------------------------------
 */
Datum
kmer_length(PG_FUNCTION_ARGS)
{
    Kmer *k;
    int   n;

    k = (Kmer *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    check_kmer_consistency(k);

    n = kmer_length_internal(k);

    PG_RETURN_INT32(n);
}

char
kmer_get_base(const Kmer *k, int i)
{
    int           byte_index = i / 4;
    int           shift      = (3 - (i % 4)) * 2;
    unsigned char v          = (k->data[byte_index] >> shift) & 0x03;
    static const char table[4] = { 'A', 'C', 'G', 'T' };
    return table[v];
}