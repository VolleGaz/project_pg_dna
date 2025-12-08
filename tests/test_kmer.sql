SET client_min_messages = WARNING;

DROP EXTENSION IF EXISTS pg_dna CASCADE;
CREATE EXTENSION pg_dna;

SELECT '--- Input/Output ---' AS section;
SELECT 'ACGT'::kmer;
SELECT 'acgt'::kmer;

SELECT '--- Length ---' AS section;
SELECT length('AC'::kmer);
SELECT length('ACGTACGTACGTACGTACGTACGTACGTACGT'::kmer);

SELECT '--- Aliases ---' AS section;
SELECT equals('ACGT'::kmer, 'ACGT'::kmer);
SELECT equals('ACGT'::kmer, 'TGCA'::kmer);
SELECT starts_with('AC'::kmer, 'ACGT'::kmer);  -- prefix first
SELECT starts_with('GT'::kmer, 'ACGT'::kmer);

SELECT '--- Errors ---' AS section;

DO $$
BEGIN
    BEGIN
        PERFORM repeat('C', 40)::kmer;
        RAISE EXCEPTION 'ERROR EXPECTED: length exceeds maximum';
    EXCEPTION WHEN others THEN
        -- OK
    END;
END;
$$;

DO $$
BEGIN
    BEGIN
        PERFORM 'AXGT'::kmer;
        RAISE EXCEPTION 'ERROR EXPECTED: invalid base';
    EXCEPTION WHEN others THEN
        -- OK
    END;
END;
$$;
