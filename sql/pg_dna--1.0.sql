-- SQL script for pg_dna extension version 1.0

-- 1. Create a shell type so that we can reference it in function signatures
CREATE TYPE dna;

-- 2. Declare the input function
CREATE FUNCTION dna_in(cstring)
RETURNS dna
AS 'pg_dna', 'dna_in'
LANGUAGE C IMMUTABLE STRICT;

-- 3. Declare the output function
CREATE FUNCTION dna_out(dna)
RETURNS cstring
AS 'pg_dna', 'dna_out'
LANGUAGE C IMMUTABLE STRICT;

-- 4. Complete the DNA type definition
CREATE TYPE dna (
    INPUT = dna_in,
    OUTPUT = dna_out,
    INTERNALLENGTH = VARIABLE,
    STORAGE = EXTENDED
);

-- 5. Utility: length of a DNA sequence
CREATE FUNCTION dna_length(dna)
RETURNS integer
AS 'pg_dna', 'dna_length'
LANGUAGE C IMMUTABLE STRICT;

COMMENT ON TYPE dna IS 'DNA sequence type stored like text (A/C/G/T only)';
