#include "postgres.h"
#if PG_VERSION_NUM >= 160000
#include "varatt.h"
#endif
#include "fmgr.h"
#include "utils/varlena.h"
#include "utils/builtins.h"

#include "kmer.h"
#include "qkmer.h"

#include <string.h>

PG_FUNCTION_INFO_V1(kmer_eq);
PG_FUNCTION_INFO_V1(kmer_starts_with);
PG_FUNCTION_INFO_V1(qkmer_contains);



/* qkmer helpers: length and consistency (same as qkmer.c logic) */
static inline int
qkmer_length_internal(const QKmer *q)
{
    Size size   = VARSIZE_ANY(q);
    Size header = offsetof(QKmer, data);

    if (size < header)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("qkmer value is corrupted")));

    return (int) (size - header);
}

static inline void
check_qkmer_consistency_ops(const QKmer *q)
{
    int  n        = qkmer_length_internal(q);
    Size expected = offsetof(QKmer, data) + n;

    if (n <= 0 || n > QKMER_MAX_LENGTH)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("qkmer value has unreasonable length: %d", n)));

    if (VARSIZE_ANY(q) != expected)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("qkmer value has invalid internal size")));
}

/* Does a single qkmer code match a concrete base A/C/G/T ? */
static inline bool
qbase_matches(char q, char base)
{
    switch (q)
    {
        case 'A': return base == 'A';
        case 'C': return base == 'C';
        case 'G': return base == 'G';
        case 'T': return base == 'T';

        case 'N': return (base == 'A' || base == 'C' || base == 'G' || base == 'T');
        case 'R': return (base == 'A' || base == 'G');
        case 'Y': return (base == 'C' || base == 'T');
        case 'S': return (base == 'G' || base == 'C');
        case 'W': return (base == 'A' || base == 'T');
        case 'K': return (base == 'G' || base == 'T');
        case 'M': return (base == 'A' || base == 'C');
        case 'B': return (base == 'C' || base == 'G' || base == 'T');      /* not A */
        case 'D': return (base == 'A' || base == 'G' || base == 'T');      /* not C */
        case 'H': return (base == 'A' || base == 'C' || base == 'T');      /* not G */
        case 'V': return (base == 'A' || base == 'C' || base == 'G');      /* not T */

        default:
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_CORRUPTED),
                     errmsg("qkmer contains invalid IUPAC code '%c'", q)));
            return false;
    }
}

/* ------------------------------------------------------------------------
 * kmer_eq(kmer, kmer) -> boolean
 * ------------------------------------------------------------------------ */
Datum
kmer_eq(PG_FUNCTION_ARGS)
{
    Kmer *a = (Kmer *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    Kmer *b = (Kmer *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

    int na = a->length;
    int nb = b->length;

    if (na != nb)
        PG_RETURN_BOOL(false);

    for (int i = 0; i < na; i++)
    {
        char ca = kmer_get_base(a, i);
        char cb = kmer_get_base(b, i);
        if (ca != cb)
            PG_RETURN_BOOL(false);
    }

    PG_RETURN_BOOL(true);
}

/* ------------------------------------------------------------------------
 * kmer_starts_with(prefix kmer, value kmer) -> boolean
 * Spec: error if prefix length > value length.
 * ------------------------------------------------------------------------ */
Datum
kmer_starts_with(PG_FUNCTION_ARGS)
{
    Kmer *value = (Kmer *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    Kmer *prefix  = (Kmer *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

    int np = prefix->length;
    int nv = value->length;

    if (np > nv)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("starts_with: prefix length %d exceeds kmer length %d",
                        np, nv)));

    for (int i = 0; i < np; i++)
    {
        char cp = kmer_get_base(prefix, i);
        char cv = kmer_get_base(value, i);
        if (cp != cv)
            PG_RETURN_BOOL(false);
    }

    PG_RETURN_BOOL(true);
}

/* ------------------------------------------------------------------------
 * qkmer_contains(pattern qkmer, value kmer) -> boolean
 * Spec: error if lengths differ.
 * ------------------------------------------------------------------------ */
Datum
qkmer_contains(PG_FUNCTION_ARGS)
{
    QKmer *pattern = (QKmer *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    Kmer  *value   = (Kmer  *) PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

    check_qkmer_consistency_ops(pattern);

    int np = qkmer_length_internal(pattern);
    int nv = value->length;

    if (np != nv)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("contains: pattern length %d does not match kmer length %d",
                        np, nv)));

    for (int i = 0; i < np; i++)
    {
        char q = pattern->data[i];        /* already uppercase IUPAC */
        char b = kmer_get_base(value, i); /* A/C/G/T */

        if (!qbase_matches(q, b))
            PG_RETURN_BOOL(false);
    }

    PG_RETURN_BOOL(true);
}
