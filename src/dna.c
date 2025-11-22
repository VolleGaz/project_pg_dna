#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"

#include <ctype.h>

PG_FUNCTION_INFO_V1(dna_in);
PG_FUNCTION_INFO_V1(dna_out);

/*
 * dna_in(cstring) : input function
 * Validates that the input only contains characters A, C, G, T
 * (case-insensitive), normalizes them to uppercase, and stores
 * the sequence using the same internal representation as 'text'.
 */
Datum
dna_in(PG_FUNCTION_ARGS)
{
    char   *input = PG_GETARG_CSTRING(0);
    int     len = strlen(input);

    /* Work on a copy so we can normalize to uppercase */
    char   *buf = palloc(len + 1);

    for (int i = 0; i < len; i++)
    {
        char c = toupper((unsigned char) input[i]);

        switch (c)
        {
            case 'A':
            case 'C':
            case 'G':
            case 'T':
                buf[i] = c;
                break;
            default:
                ereport(ERROR,
                        (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                         errmsg("invalid DNA base: '%c' (allowed: A,C,G,T only)", input[i])));
        }
    }
    buf[len] = '\0';

    /* Let PostgreSQL build a proper varlena (text) value */
    text *result = cstring_to_text_with_len(buf, len);
    pfree(buf);

    PG_RETURN_TEXT_P(result);
}

/*
 * dna_out(dna) : output function
 * Just converts the internal 'text-like' representation back to cstring.
 */
Datum
dna_out(PG_FUNCTION_ARGS)
{
    Datum  arg = PG_GETARG_DATUM(0);
    text  *txt = DatumGetTextPP(arg);  /* handles toasted values safely */
    char  *out = text_to_cstring(txt);

    PG_RETURN_CSTRING(out);
}
