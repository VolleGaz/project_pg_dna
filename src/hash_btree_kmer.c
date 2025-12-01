#include "postgres.h"
#include "fmgr.h"
#include "common/hashfn.h"
#include "kmer.h"

PG_FUNCTION_INFO_V1(kmer_hash);

Datum
kmer_hash(PG_FUNCTION_ARGS)
{
    Kmer *k = (Kmer *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
    int packed_bytes = KMER_PACKED_BYTES(k->length);

    uint32 h = hash_bytes((unsigned char *) k->data, packed_bytes);

    PG_RETURN_UINT32(h);
}



