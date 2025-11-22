#include "postgres.h"
#include "varatt.h"
#include "fmgr.h"
#include "utils/varlena.h"
#include "utils/builtins.h"
#include "dna.h"

#include <ctype.h>
#include <string.h>


PG_FUNCTION_INFO_V1(dna_in);
PG_FUNCTION_INFO_V1(dna_out);
PG_FUNCTION_INFO_V1(dna_length);

/* Encode base to 2 bits */
static unsigned char
encode_base(char c)
{
    switch (c)
    {
        case 'A': case 'a': return 0;
        case 'C': case 'c': return 1;
        case 'G': case 'g': return 2;
        case 'T': case 't': return 3;
        default:
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                     errmsg("invalid DNA base: '%c' (allowed: A,C,G,T only)", c)));
            return 0;
    }
}

/* Decode 2 bits to base */
static char
decode_base(unsigned char b)
{
    static const char map[4] = { 'A', 'C', 'G', 'T' };
    return map[b & 0x3];
}

/* Input function */
Datum
dna_in(PG_FUNCTION_ARGS)
{
    char   *input = PG_GETARG_CSTRING(0);
    uint32  n     = (uint32) strlen(input);
    uint32  packed_bytes = DNA_PACKED_BYTES(n);

    /* Correct varlen allocation */
    Size size = offsetof(Dna, data) + packed_bytes;

    Dna *result = (Dna *) palloc(size);

    SET_VARSIZE(result, size);   /* sets vl_len_ */
    result->length = n;

    memset(result->data, 0, packed_bytes);

    for (uint32 i = 0; i < n; i++)
    {
        unsigned char b = encode_base(input[i]);

        uint32 byte_index = i / 4;
        uint32 shift      = (3 - (i % 4)) * 2;

        result->data[byte_index] |= (b << shift);
    }

    PG_RETURN_POINTER(result);
}

/* Output function */
Datum
dna_out(PG_FUNCTION_ARGS)
{
    Dna    *dna = (Dna *) PG_GETARG_POINTER(0);
    uint32  n   = dna->length;

    char *buf = (char *) palloc(n + 1);

    for (uint32 i = 0; i < n; i++)
    {
        uint32 byte_index = i / 4;
        uint32 shift      = (3 - (i % 4)) * 2;

        buf[i] = decode_base((dna->data[byte_index] >> shift) & 0x3);
    }

    buf[n] = '\0';

    PG_RETURN_CSTRING(buf);
}

/* Length function */
Datum
dna_length(PG_FUNCTION_ARGS)
{
    Dna *dna = (Dna *) PG_GETARG_POINTER(0);
    PG_RETURN_INT32((int32) dna->length);
}
