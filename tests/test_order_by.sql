-- Tests ORDER BY count and ORDER BY kmer (using kmer btree operators)

DROP EXTENSION IF EXISTS pg_dna CASCADE;
CREATE EXTENSION pg_dna;

\pset format aligned
\pset tuples_only off
\pset border 1
\pset pager off

-- Test 1: ORDER BY count DESC, then kmer ASC (uses kmer_cmp)


\echo 'Test ORDER BY count DESC, kmer ASC (ACGTACGT, k=3)'

SELECT k.kmer, count(*) AS cnt
FROM generate_kmers('ACGTACGT', 3) AS k(kmer)
GROUP BY k.kmer
ORDER BY cnt DESC, k.kmer;

-- Test 2: ORDER BY kmer ASC (btree operator class)

\echo 'Test ORDER BY kmer ASC (VALUES)'

SELECT v.kmer
FROM (VALUES
    ('ACGTA'::kmer),
    ('ACGTT'::kmer),
    ('ACGTC'::kmer),
    ('ACG'::kmer),
    ('ACT'::kmer)
) AS v(kmer)
ORDER BY v.kmer;

-- Test 3: ORDER BY kmer DESC (btree operator class)

\echo 'Test ORDER BY kmer DESC (VALUES)'

SELECT v.kmer
FROM (VALUES
    ('ACGTA'::kmer),
    ('ACGTT'::kmer),
    ('ACGTC'::kmer),
    ('ACG'::kmer),
    ('ACT'::kmer)
) AS v(kmer)
ORDER BY v.kmer DESC;
