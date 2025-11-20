#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(hello_dna);

Datum
hello_dna(PG_FUNCTION_ARGS)
{
    text *txt = cstring_to_text("pg_dna extension loaded");
    PG_RETURN_TEXT_P(txt);
}

