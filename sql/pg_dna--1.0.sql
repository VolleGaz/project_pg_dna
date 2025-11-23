-- SQL script for pg_dna extension version 1.0
-- 1. Create a shell type so that we can reference it in function signatures
CREATE TYPE dna;
-- 2. Declare the input function
CREATE FUNCTION dna_in(cstring) RETURNS dna AS 'pg_dna',
'dna_in' LANGUAGE C IMMUTABLE STRICT;
-- 3. Declare the output function
CREATE FUNCTION dna_out(dna) RETURNS cstring AS 'pg_dna',
'dna_out' LANGUAGE C IMMUTABLE STRICT;
-- 4. Complete the DNA type definition
CREATE TYPE dna (
    INPUT = dna_in,
    OUTPUT = dna_out,
    INTERNALLENGTH = VARIABLE,
    STORAGE = EXTENDED
);
-- 5. Utility: length of a DNA sequence
CREATE FUNCTION dna_length(dna) RETURNS integer AS 'pg_dna',
'dna_length' LANGUAGE C IMMUTABLE STRICT;
COMMENT ON TYPE dna IS 'DNA sequence type stored like text (A/C/G/T only)';
-- 5b. Polymorphic-style length(dna) wrapper, to match length(text)
CREATE FUNCTION length(dna) RETURNS integer AS 'pg_dna',
'dna_length' LANGUAGE C IMMUTABLE STRICT;
--6. Utility: get nucleotide at specific position (1-based index)
CREATE FUNCTION dna_get(dna, integer) RETURNS text AS 'pg_dna',
'dna_get' LANGUAGE C IMMUTABLE STRICT;
-- ============================================
-- kmer type
-- ============================================
-- 1. Shell type
CREATE TYPE kmer;
-- 2. I/O functions
CREATE FUNCTION kmer_in(cstring) RETURNS kmer AS 'pg_dna',
'kmer_in' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION kmer_out(kmer) RETURNS cstring AS 'pg_dna',
'kmer_out' LANGUAGE C IMMUTABLE STRICT;
-- 3. Complete kmer type definition
CREATE TYPE kmer (
    INPUT = kmer_in,
    OUTPUT = kmer_out,
    INTERNALLENGTH = VARIABLE,
    STORAGE = EXTENDED
);
-- 4. length(kmer)
CREATE FUNCTION kmer_length(kmer) RETURNS integer AS 'pg_dna',
'kmer_length' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION length(kmer) RETURNS integer AS 'pg_dna',
'kmer_length' LANGUAGE C IMMUTABLE STRICT;