-- Tests for generate_kmers(dna, k)

SET client_min_messages = WARNING;

DROP EXTENSION IF EXISTS pg_dna CASCADE;
CREATE EXTENSION pg_dna;

SELECT '--- basic windows ---' AS section;

DO $$
DECLARE
    res text[];
BEGIN
    SELECT array_agg(kmer::text) INTO res
    FROM generate_kmers('ACGTAC'::dna, 3) AS k(kmer);

    IF res IS DISTINCT FROM ARRAY['ACG','CGT','GTA','TAC']::text[] THEN
        RAISE EXCEPTION 'unexpected k-mers: %', res;
    END IF;
END;
$$;

SELECT '--- exact length ---' AS section;

DO $$
DECLARE
    res text[];
BEGIN
    SELECT array_agg(kmer::text) INTO res
    FROM generate_kmers('ACGT'::dna, 4) AS k(kmer);

    IF res IS DISTINCT FROM ARRAY['ACGT']::text[] THEN
        RAISE EXCEPTION 'unexpected k-mers for exact length: %', res;
    END IF;
END;
$$;

SELECT '--- shorter than k ---' AS section;

DO $$
DECLARE
    res text[];
BEGIN
    SELECT coalesce(array_agg(kmer::text), '{}'::text[]) INTO res
    FROM generate_kmers('AC'::dna, 3) AS k(kmer);

    IF res IS DISTINCT FROM '{}'::text[] THEN
        RAISE EXCEPTION 'expected empty result, got %', res;
    END IF;
END;
$$;

SELECT '--- error cases ---' AS section;

DO $$
BEGIN
    BEGIN
        PERFORM generate_kmers('ACGT'::dna, 0);
        RAISE EXCEPTION 'ERROR EXPECTED: k must be positive';
    EXCEPTION WHEN others THEN
        -- OK
    END;
END;
$$;

DO $$
BEGIN
    BEGIN
        PERFORM generate_kmers('ACGT'::dna, 33);
        RAISE EXCEPTION 'ERROR EXPECTED: k exceeds maximum length';
    EXCEPTION WHEN others THEN
        -- OK
    END;
END;
$$;


SELECT '--- DONE ---' AS section;
