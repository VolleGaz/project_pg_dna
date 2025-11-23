#include "postgres.h"
#if PG_VERSION_NUM >= 160000
#include "varatt.h"
#endif
#include "fmgr.h"
#include "utils/varlena.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "dna.h"

#include <ctype.h>
#include <string.h>

/*
 * IMPORTANT:
 * PG_MODULE_MAGIC is defined in pg_dna.c, not here.
 */

PG_FUNCTION_INFO_V1(dna_in);
PG_FUNCTION_INFO_V1(dna_out);
PG_FUNCTION_INFO_V1(dna_length);
PG_FUNCTION_INFO_V1(dna_get);

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

/*
 * Encode one base (char) into 2 bits (0..3).
 * Accepts both upper- and lowercase A/C/G/T.
 * Raises a PostgreSQL ERROR on invalid character.
 */
static unsigned char
encode_base(char c)
{
    unsigned char uc;

    uc = (unsigned char) c;
    uc = (unsigned char) toupper(uc);

    switch (uc)
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
                     errmsg("invalid DNA base: '%c' (allowed: A,C,G,T only)", c)));
            /* not reached */
            return 0;
    }
}

/*
 * Decode 2 bits (0..3) back into a base character.
 * This should never see values outside 0..3 if the data is consistent.
 */
static char
decode_base(unsigned char b)
{
    static const char map[4] = { 'A', 'C', 'G', 'T' };

    b &= 0x03;
    return map[b];
}

/*
 * Check that the internal varlena value is consistent:
 *  - size is large enough to contain header + length + packed data
 *  - length is not absurdly large
 * Raises ERROR if something looks corrupted (e.g. malformed toast value).
 */
static void
check_dna_consistency(const Dna *dna)
{
    Size size;
    uint32 n;
    uint32 packed_bytes;
    Size min_size;

    size = VARSIZE_ANY(dna);    /* full varlena size */
    n = dna->length;

    if (n > DNA_MAX_LENGTH)
    {
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("dna value has unreasonable length: %u", n)));
    }

    packed_bytes = DNA_PACKED_BYTES(n);
    min_size = offsetof(Dna, data) + packed_bytes;

    if (size < min_size)
    {
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("dna value is corrupted: size %zu too small for length %u",
                        (size_t) size, n)));
    }
}

/* -------------------------------------------------------------------------
 * Input function: dna_in(cstring) -> dna
 * ------------------------------------------------------------------------- */

Datum
dna_in(PG_FUNCTION_ARGS)
{
    char   *input;
    size_t  input_len;
    uint32  n;
    uint32  packed_bytes;
    Size    size;
    Dna    *result;
    uint32  i;

    input = PG_GETARG_CSTRING(0);
    input_len = strlen(input);

    if (input_len > DNA_MAX_LENGTH)
    {
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("DNA sequence too long (%zu bases, max is %u)",
                        input_len, DNA_MAX_LENGTH)));
    }

    n = (uint32) input_len;
    packed_bytes = DNA_PACKED_BYTES(n);

    /*
     * Compute total size of the varlena structure.
     * We use offsetof(Dna, data) to get the offset of the data[] field,
     * then add the number of packed bytes.
     */
    size = offsetof(Dna, data) + packed_bytes;

    if (size > MaxAllocSize)
    {
        ereport(ERROR,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                 errmsg("DNA value size exceeds maximum allocatable size")));
    }

    result = (Dna *) palloc(size);

    /* Set varlena header */
    SET_VARSIZE(result, size);

    /* Store logical length in bases */
    result->length = n;

    /* Initialize packed bytes to zero */
    if (packed_bytes > 0)
        memset(result->data, 0, packed_bytes);

    /*
     * Pack bases: 4 bases per byte, big-endian inside each byte:
     *   bits 7..6 => base 0
     *   bits 5..4 => base 1
     *   bits 3..2 => base 2
     *   bits 1..0 => base 3
     */
    for (i = 0; i < n; i++)
    {
        unsigned char b;
        uint32        byte_index;
        uint32        shift;

        b = encode_base(input[i]);

        byte_index = i / 4;
        shift      = (3 - (i % 4)) * 2;

        result->data[byte_index] |= (unsigned char) (b << shift);
    }

    PG_RETURN_POINTER(result);
}

/* -------------------------------------------------------------------------
 * Output function: dna_out(dna) -> cstring
 * ------------------------------------------------------------------------- */

Datum
dna_out(PG_FUNCTION_ARGS)
{
    Datum  arg;
    Dna   *dna;
    uint32 n;
    uint32 i;
    char  *buf;

    /*
     * Obtain a de-toasted, packed representation of the varlena value.
     * The returned pointer may point to a copied value that we own, or to
     * a read-only buffer; we must not pfree(dna).
     */
    arg = PG_GETARG_DATUM(0);
    dna = (Dna *) PG_DETOAST_DATUM(arg);

    /* Sanity check on internal structure. */
    check_dna_consistency(dna);

    n = dna->length;

    buf = (char *) palloc(n + 1);

    for (i = 0; i < n; i++)
    {
        uint32 byte_index;
        uint32 shift;
        unsigned char packed;

        byte_index = i / 4;
        shift      = (3 - (i % 4)) * 2;

        packed = (unsigned char) ((dna->data[byte_index] >> shift) & 0x03);
        buf[i] = decode_base(packed);
    }

    buf[n] = '\0';

    PG_RETURN_CSTRING(buf);
}

/* -------------------------------------------------------------------------
 * dna_length(dna) -> integer
 * ------------------------------------------------------------------------- */

Datum
dna_length(PG_FUNCTION_ARGS)
{
    Datum  arg;
    Dna   *dna;

    arg = PG_GETARG_DATUM(0);
    dna = (Dna *) PG_DETOAST_DATUM(arg);

    check_dna_consistency(dna);

    PG_RETURN_INT32((int32) dna->length);
}

/* -------------------------------------------------------------------------
 * dna_get(dna, integer) -> text (single character)
 * ------------------------------------------------------------------------- */

Datum
dna_get(PG_FUNCTION_ARGS)
{
    Datum  arg;
    Dna   *dna;
    int32  idx;
    uint32 n;
    uint32 i;
    uint32 byte_index;
    uint32 shift;
    unsigned char packed;
    char   ch;
    text  *result_text;

    arg = PG_GETARG_DATUM(0);
    dna = (Dna *) PG_DETOAST_DATUM(arg);

    check_dna_consistency(dna);

    idx = PG_GETARG_INT32(1);
    n   = dna->length;

    if (idx < 1 || (uint32) idx > n)
    {
        ereport(ERROR,
                (errcode(ERRCODE_ARRAY_ELEMENT_ERROR),
                 errmsg("dna_get: index %d out of bounds (valid range: 1..%u)",
                        idx, n)));
    }

    /* Convert to 0-based index */
    i = (uint32) (idx - 1);

    byte_index = i / 4;
    shift      = (3 - (i % 4)) * 2;

    packed = (unsigned char) ((dna->data[byte_index] >> shift) & 0x03);
    ch = decode_base(packed);

    result_text = cstring_to_text_with_len(&ch, 1);

    PG_RETURN_TEXT_P(result_text);
}
