
-- Tests for the pg_dna extension (DNA type)


SET client_min_messages = WARNING;

DROP EXTENSION IF EXISTS pg_dna CASCADE;
CREATE EXTENSION pg_dna;

SELECT '--- Input/Output ---' AS section;

SELECT 'ACGT'::dna AS dna_value, dna_out('ACGT'::dna) AS out_value;
SELECT 'acgt'::dna AS dna_value, dna_out('acgt'::dna) AS out_value;
SELECT 'AaCcGgTt'::dna AS dna_value, dna_out('AaCcGgTt'::dna) AS out_value;

-- Expected input errors (handled so that test suite does not stop)

SELECT '--- Invalid Input ---' AS section;

DO $$
BEGIN
    BEGIN
        PERFORM 'XCGT'::dna;
        RAISE EXCEPTION 'ERROR EXPECTED: invalid base X';
    EXCEPTION WHEN others THEN
        -- OK
    END;
END;
$$;

DO $$
BEGIN
    BEGIN
        PERFORM 'ACNT'::dna;
        RAISE EXCEPTION 'ERROR EXPECTED: invalid base N';
    EXCEPTION WHEN others THEN
        -- OK
    END;
END;
$$;

DO $$
BEGIN
    BEGIN
        PERFORM '1234'::dna;
        RAISE EXCEPTION 'ERROR EXPECTED: invalid numeric chars';
    EXCEPTION WHEN others THEN
        -- OK
    END;
END;
$$;

---------------------------------------------------------------
SELECT '--- Length ---' AS section;

SELECT dna_length('A'::dna);
SELECT dna_length('AC'::dna);
SELECT dna_length('ACGTACGT'::dna);
SELECT dna_length('AAAAAAAAAA'::dna);

SELECT '--- dna_get ---' AS section;

SELECT dna_get('ACGTAC'::dna, 1);
SELECT dna_get('ACGTAC'::dna, 2);
SELECT dna_get('ACGTAC'::dna, 3);
SELECT dna_get('ACGTAC'::dna, 6);

-- dna_get errors expected

DO $$
BEGIN
    BEGIN
        PERFORM dna_get('A'::dna, 2);
        RAISE EXCEPTION 'ERROR EXPECTED: index too large';
    EXCEPTION WHEN others THEN
        -- OK
    END;
END;
$$;

DO $$
BEGIN
    BEGIN
        PERFORM dna_get('ACGTAC'::dna, 0);
        RAISE EXCEPTION 'ERROR EXPECTED: index too small';
    EXCEPTION WHEN others THEN
    END;
END;
$$;

DO $$
BEGIN
    BEGIN
        PERFORM dna_get('ACGTAC'::dna, 7);
        RAISE EXCEPTION 'ERROR EXPECTED: index out of range';
    EXCEPTION WHEN others THEN
        -- OK
    END;
END;
$$;

SELECT '--- Large sequences ---' AS section;

SELECT dna_length(repeat('ACGT', 25)::dna);
SELECT dna_get(repeat('ACGT', 25)::dna, 87);

SELECT '--- DONE ---' AS section;
