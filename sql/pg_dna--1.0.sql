-- SQL script for pg_dna extension version 1.0
--  Create a shell type so that we can reference it in function signatures
CREATE TYPE dna;
--  Declare the input function
CREATE FUNCTION dna_in(cstring) RETURNS dna AS 'pg_dna',
'dna_in' LANGUAGE C IMMUTABLE STRICT;
--  Declare the output function
CREATE FUNCTION dna_out(dna) RETURNS cstring AS 'pg_dna',
'dna_out' LANGUAGE C IMMUTABLE STRICT;
--  Complete the DNA type definition
CREATE TYPE dna (
    INPUT = dna_in,
    OUTPUT = dna_out,
    INTERNALLENGTH = VARIABLE,
    STORAGE = EXTENDED
);
-- Allow implicit/assignment casts from text literals
CREATE CAST (text AS dna) WITH FUNCTION dna_in(cstring) AS ASSIGNMENT;
--  Utility: length of a DNA sequence
CREATE FUNCTION dna_length(dna) RETURNS integer AS 'pg_dna',
'dna_length' LANGUAGE C IMMUTABLE STRICT;
COMMENT ON TYPE dna IS 'DNA sequence type stored like text (A/C/G/T only)';
-- . Polymorphic-style length(dna) wrapper, to match length(text)
CREATE FUNCTION length(dna) RETURNS integer AS 'pg_dna',
'dna_length' LANGUAGE C IMMUTABLE STRICT;
-- Utility: get nucleotide at specific position (1-based index)
CREATE FUNCTION dna_get(dna, integer) RETURNS text AS 'pg_dna',
'dna_get' LANGUAGE C IMMUTABLE STRICT;
-- kmer type
-- 
CREATE TYPE kmer;
-- I/O functions
CREATE FUNCTION kmer_in(cstring) RETURNS kmer AS 'pg_dna',
'kmer_in' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION kmer_out(kmer) RETURNS cstring AS 'pg_dna',
'kmer_out' LANGUAGE C IMMUTABLE STRICT;
--  Complete kmer type definition
CREATE TYPE kmer (
    INPUT = kmer_in,
    OUTPUT = kmer_out,
    INTERNALLENGTH = VARIABLE,
    STORAGE = EXTENDED
);
-- Allow implicit/assignment casts from text literals
CREATE CAST (text AS kmer) WITH FUNCTION kmer_in(cstring) AS ASSIGNMENT;
--  length(kmer)
CREATE FUNCTION kmer_length(kmer) RETURNS integer AS 'pg_dna',
'kmer_length' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION length(kmer) RETURNS integer AS 'pg_dna',
'kmer_length' LANGUAGE C IMMUTABLE STRICT;
-- qkmer type
CREATE TYPE qkmer;
-- I/O functions
CREATE FUNCTION qkmer_in(cstring) RETURNS qkmer AS 'pg_dna',
'qkmer_in' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION qkmer_out(qkmer) RETURNS cstring AS 'pg_dna',
'qkmer_out' LANGUAGE C IMMUTABLE STRICT;
-- Complete qkmer type definition
CREATE TYPE qkmer (
    INPUT = qkmer_in,
    OUTPUT = qkmer_out,
    INTERNALLENGTH = VARIABLE,
    STORAGE = EXTENDED
);
-- Allow implicit/assignment casts from text literals
CREATE CAST (text AS qkmer) WITH FUNCTION qkmer_in(cstring) AS ASSIGNMENT;
-- length(qkmer)
CREATE FUNCTION qkmer_length(qkmer) RETURNS integer AS 'pg_dna',
'qkmer_length' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION length(qkmer) RETURNS integer AS 'pg_dna',
'qkmer_length' LANGUAGE C IMMUTABLE STRICT;
-- kmer and qkmer operators
-- Functions
CREATE FUNCTION kmer_eq(kmer, kmer) RETURNS boolean AS 'pg_dna',
'kmer_eq' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION kmer_starts_with(kmer, kmer) RETURNS boolean AS 'pg_dna',
'kmer_starts_with' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION qkmer_contains(qkmer, kmer) RETURNS boolean AS 'pg_dna',
'qkmer_contains' LANGUAGE C IMMUTABLE STRICT;

-- Assignment-friendly aliases (argument order matches the project PDF)
CREATE FUNCTION equals(kmer, kmer) RETURNS boolean AS 'pg_dna',
'kmer_eq' LANGUAGE C IMMUTABLE STRICT;

-- PDF uses starts_with(prefix, value); internal function expects (value, prefix)
CREATE FUNCTION starts_with(kmer, kmer) RETURNS boolean AS $$
    SELECT kmer_starts_with($2, $1);
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE FUNCTION contains(qkmer, kmer) RETURNS boolean AS $$
    SELECT qkmer_contains($1, $2);
$$ LANGUAGE SQL IMMUTABLE STRICT;
-- generate_kmers(dna, k) -> SETOF kmer
CREATE FUNCTION generate_kmers(dna, integer)
RETURNS SETOF kmer AS 'pg_dna', 'generate_kmers'
LANGUAGE C IMMUTABLE STRICT;
-- Operators


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

CREATE FUNCTION kmer_contained_by(kmer, qkmer) RETURNS boolean AS 'pg_dna',
'kmer_contained_by' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR <@ (
    LEFTARG = kmer,
    RIGHTARG = qkmer,
    PROCEDURE = kmer_contained_by,
    COMMUTATOR = '@>'
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

-- Hash function for GROUP BY and DISTINCT
CREATE FUNCTION kmer_hash(kmer)
RETURNS integer
AS 'pg_dna', 'kmer_hash'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR CLASS kmer_hash_ops
DEFAULT FOR TYPE kmer USING hash AS
    OPERATOR 1 = (kmer, kmer),
    FUNCTION 1 kmer_hash(kmer);


-- Comparing function and comparaison operators for the b-tree for ordering

CREATE FUNCTION kmer_cmp(kmer, kmer)
RETURNS integer
AS 'pg_dna', 'kmer_cmp'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION kmer_lt(kmer, kmer)
RETURNS boolean
AS $$
    SELECT kmer_cmp($1, $2) < 0;
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR < (
    LEFTARG = kmer, RIGHTARG = kmer,
    PROCEDURE = kmer_lt,
    COMMUTATOR = '>',
    NEGATOR = '>=',
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

CREATE FUNCTION kmer_le(kmer, kmer)
RETURNS boolean AS $$
    SELECT kmer_cmp($1, $2) <= 0;
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR <= (
    LEFTARG = kmer, RIGHTARG = kmer,
    PROCEDURE = kmer_le,
    COMMUTATOR = '>=',
    NEGATOR = '>',
    RESTRICT = scalarlesel,
    JOIN = scalarlejoinsel
);

CREATE FUNCTION kmer_gt(kmer, kmer)
RETURNS boolean AS $$
    SELECT kmer_cmp($1, $2) > 0;
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR > (
    LEFTARG = kmer, RIGHTARG = kmer,
    PROCEDURE = kmer_gt,
    COMMUTATOR = '<',
    NEGATOR = '<=',
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

CREATE FUNCTION kmer_ge(kmer, kmer)
RETURNS boolean AS $$
    SELECT kmer_cmp($1, $2) >= 0;
$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE OPERATOR >= (
    LEFTARG = kmer, RIGHTARG = kmer,
    PROCEDURE = kmer_ge,
    COMMUTATOR = '<=',
    NEGATOR = '<',
    RESTRICT = scalargesel,
    JOIN = scalargejoinsel
);

CREATE OPERATOR CLASS kmer_btree_ops
DEFAULT FOR TYPE kmer USING btree AS
    OPERATOR 1  <  (kmer, kmer),
    OPERATOR 2  <= (kmer, kmer),
    OPERATOR 3  =  (kmer, kmer),
    OPERATOR 4  >= (kmer, kmer),
    OPERATOR 5  >  (kmer, kmer),
    FUNCTION 1 kmer_cmp(kmer, kmer);


-- framework sp-gist

CREATE OPERATOR CLASS kmer_spgist_ops
DEFAULT FOR TYPE kmer USING spgist AS
    STORAGE kmer,

    -- 3  = operator
    -- 28 ^@ operator
    -- 10 operator  @>
    OPERATOR  3  =  (kmer, kmer),
    OPERATOR 28  ^@ (kmer, kmer),
    OPERATOR 10 <@ (kmer, qkmer),
    FUNCTION 1  spg_kmer_config           (internal, internal),
    FUNCTION 2  spg_kmer_choose           (internal, internal),
    FUNCTION 3  spg_kmer_picksplit        (internal, internal),
    FUNCTION 4  spg_kmer_inner_consistent (internal, internal),
    FUNCTION 5  spg_kmer_leaf_consistent  (internal, internal);
