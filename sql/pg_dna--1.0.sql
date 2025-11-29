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
-- ============================================
-- qkmer type
-- ============================================
-- 1. Shell type
CREATE TYPE qkmer;
-- 2. I/O functions
CREATE FUNCTION qkmer_in(cstring) RETURNS qkmer AS 'pg_dna',
'qkmer_in' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION qkmer_out(qkmer) RETURNS cstring AS 'pg_dna',
'qkmer_out' LANGUAGE C IMMUTABLE STRICT;
-- 3. Complete qkmer type definition
CREATE TYPE qkmer (
    INPUT = qkmer_in,
    OUTPUT = qkmer_out,
    INTERNALLENGTH = VARIABLE,
    STORAGE = EXTENDED
);
-- 4. length(qkmer)
CREATE FUNCTION qkmer_length(qkmer) RETURNS integer AS 'pg_dna',
'qkmer_length' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION length(qkmer) RETURNS integer AS 'pg_dna',
'qkmer_length' LANGUAGE C IMMUTABLE STRICT;
-- ============================================
-- kmer and qkmer operators
-- ============================================
-- 1. Functions
CREATE FUNCTION kmer_eq(kmer, kmer) RETURNS boolean AS 'pg_dna',
'kmer_eq' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION kmer_starts_with(kmer, kmer) RETURNS boolean AS 'pg_dna',
'kmer_starts_with' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION qkmer_contains(qkmer, kmer) RETURNS boolean AS 'pg_dna',
'qkmer_contains' LANGUAGE C IMMUTABLE STRICT;
-- generate_kmers(dna, k) → SETOF kmer
CREATE FUNCTION generate_kmers(dna, integer)
RETURNS SETOF kmer AS 'pg_dna', 'generate_kmers'
LANGUAGE C IMMUTABLE STRICT;
-- 2. Operators


-- equality: = on kmer
CREATE OPERATOR = (
    LEFTARG = kmer,
    RIGHTARG = kmer,
    PROCEDURE = kmer_eq,
    COMMUTATOR = '=',
    RESTRICT = eqsel,
    JOIN = eqjoinsel
);
-- prefix: ^@
CREATE OPERATOR ^@ (
    LEFTARG   = kmer,
    RIGHTARG  = kmer,
    PROCEDURE = kmer_starts_with
);
-- pattern: qkmer @> kmer
CREATE OPERATOR @> (
    LEFTARG = qkmer,
    RIGHTARG = kmer,
    PROCEDURE = qkmer_contains
);

-- SP-GiST support functions for kmer
CREATE FUNCTION spg_kmer_config(internal, internal)
    RETURNS void
AS 'MODULE_PATHNAME', 'spg_kmer_config'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION spg_kmer_choose(internal, internal)
    RETURNS void
AS 'MODULE_PATHNAME', 'spg_kmer_choose'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION spg_kmer_picksplit(internal, internal)
    RETURNS void
AS 'MODULE_PATHNAME', 'spg_kmer_picksplit'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION spg_kmer_inner_consistent(internal, internal)
    RETURNS void
AS 'MODULE_PATHNAME', 'spg_kmer_inner_consistent'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION spg_kmer_leaf_consistent(internal, internal)
    RETURNS void
AS 'MODULE_PATHNAME', 'spg_kmer_leaf_consistent'
LANGUAGE C IMMUTABLE STRICT;

-- framework sp-gist

CREATE OPERATOR CLASS kmer_spgist_ops
DEFAULT FOR TYPE kmer USING spgist AS
    STORAGE kmer,

    -- 3  = operator
    -- 28 ^@ operator
    OPERATOR  3  =  (kmer, kmer),
    OPERATOR 28  ^@ (kmer, kmer),

    FUNCTION 1  spg_kmer_config           (internal, internal),
    FUNCTION 2  spg_kmer_choose           (internal, internal),
    FUNCTION 3  spg_kmer_picksplit        (internal, internal),
    FUNCTION 4  spg_kmer_inner_consistent (internal, internal),
    FUNCTION 5  spg_kmer_leaf_consistent  (internal, internal);
