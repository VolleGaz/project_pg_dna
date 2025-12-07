#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "utils/memutils.h"

#include "kmer.h"

#include <string.h>

PG_FUNCTION_INFO_V1(generate_kmers);

//We call dna_out() from here
extern Datum dna_out(PG_FUNCTION_ARGS);
extern Datum kmer_in(PG_FUNCTION_ARGS);

/*
 * State carried across calls for the set-returning function
 */
typedef struct GenerateKmersState
{
    char   *dna_str;   /* plain C string representation of the DNA sequence */
    uint32  dna_len;   /* length in characters (A/C/G/T) */
    int32   k;         /* window size */
    uint32  pos;       /* current start index (0-based) */
    uint32  max_pos;   /* number of windows = dna_len - k + 1 */
} GenerateKmersState;

/*
 * generate_kmers(dna, k) → SETOF kmer
 *
 * Implementation strategy:
 *  - call dna_out(dna) to get a C string like "ACGTAC..."
 *  - slide a window of length k over that string
 *  - for each window, build a kmer by calling kmer_in(...)
 *
 * This way we never depend on the internal representation of dna.
 */
Datum
generate_kmers(PG_FUNCTION_ARGS)
{
    FuncCallContext     *funcctx;
    GenerateKmersState  *state;

    // First call: initialize SRF context
    if (SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext;
        char         *dna_str;
        char         *dna_copy;
        uint32        dna_len;
        int32         k;

        k = PG_GETARG_INT32(1);
        if (k <= 0)
            ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                     errmsg("k must be positive")));

        if (k > KMER_MAX_LENGTH)
            ereport(ERROR,
                    (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                     errmsg("k-mer length %d exceeds maximum %d",
                            k, KMER_MAX_LENGTH)));

        funcctx = SRF_FIRSTCALL_INIT();

        // Switch to multi-call context so our state survives across calls
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        // Convert dna value to cstring using dna_out(dna)
        dna_str = DatumGetCString(DirectFunctionCall1(dna_out,
                                                      PG_GETARG_DATUM(0)));

        // Copy it into this memory context
        dna_copy = pstrdup(dna_str);
        dna_len  = (uint32) strlen(dna_copy);

        state = (GenerateKmersState *) palloc(sizeof(GenerateKmersState));
        state->dna_str = dna_copy;
        state->dna_len = dna_len;
        state->k       = k;
        state->pos     = 0;

        if (dna_len >= (uint32) k)
            state->max_pos = dna_len - (uint32) k + 1;
        else
            state->max_pos = 0;      // no windows

        funcctx->user_fctx = state;

        MemoryContextSwitchTo(oldcontext);
    }

    // Next calls: produce one k-mer per call
    funcctx = SRF_PERCALL_SETUP();
    state   = (GenerateKmersState *) funcctx->user_fctx;


    if (state->pos >= state->max_pos)
        SRF_RETURN_DONE(funcctx);

    // Build k-mer substring as cstring
    {
        uint32 start = state->pos;
        uint32 end   = start + (uint32) state->k;   // exclusive
        char  *buf   = (char *) palloc(state->k + 1);
        uint32 i;

        for (i = start; i < end; i++)
            buf[i - start] = state->dna_str[i];

        buf[state->k] = '\0';

        // Turn substring into a kmer value using the existing kmer_in()
        Datum kmer_datum = DirectFunctionCall1(kmer_in, CStringGetDatum(buf));

        state->pos++;

        SRF_RETURN_NEXT(funcctx, kmer_datum);
    }
}
