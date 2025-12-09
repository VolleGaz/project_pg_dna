SET client_min_messages = WARNING;

DROP EXTENSION IF EXISTS pg_dna CASCADE;
CREATE EXTENSION pg_dna;

SELECT '--- Input/Output ---' AS section;
SELECT 'ACGT'::qkmer;
SELECT 'nryswkmbdhv'::qkmer;
SELECT 'acgt'::qkmer;

SELECT '--- Length ---' AS section;
SELECT length('AC'::qkmer);
SELECT length('NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN'::qkmer);
-- 32

SELECT '--- Contains alias ---' AS section;
SELECT contains('AN'::qkmer, 'AC'::kmer);
SELECT contains('AN'::qkmer, 'GT'::kmer);

SELECT '--- Operator literal casting ---' AS section;
SELECT 'ANGTA' @> 'ATGTA'::kmer;
SELECT 'ANGTA' @> 'ACGTA'::kmer;

SELECT '--- Errors ---' AS section;

DO $$
BEGIN
    BEGIN
        PERFORM ''::qkmer;
        RAISE EXCEPTION 'ERROR EXPECTED: empty qkmer';
    EXCEPTION WHEN others THEN
        -- OK
    END;
END;
$$;

DO $$
BEGIN
    BEGIN
        PERFORM 'Z'::qkmer;
        RAISE EXCEPTION 'ERROR EXPECTED: invalid base';
    EXCEPTION WHEN others THEN
        -- OK
    END;
END;
$$;

DO $$
BEGIN
    BEGIN
        PERFORM repeat('N', 33)::qkmer;
        RAISE EXCEPTION 'ERROR EXPECTED: length exceeds maximum';
    EXCEPTION WHEN others THEN
        -- OK
    END;
END;
$$;
