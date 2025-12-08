-- Tests k-mer counting with GROUP BY

DROP EXTENSION IF EXISTS pg_dna CASCADE;
CREATE EXTENSION pg_dna;

\pset format aligned
\pset tuples_only off
\pset border 1
\pset pager off

-- Test 1: GROUP BY on generate_kmers with unique result row

\echo 'Test group by, single sequence (AAAAA, k=3)'

SELECT k.kmer, count(*) AS cnt
FROM generate_kmers('AAAAA', 3) AS k(kmer)
GROUP BY k.kmer
ORDER BY cnt;

-- Test 2: total / distinct / unique counts 

\echo 'Test group by counts summary (ACGTACGT, k=3)'

WITH kmers AS (
    SELECT k.kmer, count(*) AS cnt
    FROM generate_kmers('ACGTACGT', 3) AS k(kmer)
    GROUP BY k.kmer
)
SELECT sum(cnt) AS total_count,
       count(*) AS distinct_count,
       count(*) FILTER (WHERE cnt = 1) AS unique_count
FROM kmers;

-- Test 3: GROUP BY with multiple kmers and distinct counts

\echo 'Test group by with distinct counts (manual VALUES)'

WITH kmers(kmer) AS (
    VALUES
        ('AAA'::kmer),           
        ('CCC'::kmer), ('CCC'::kmer),   
        ('GGG'::kmer), ('GGG'::kmer), ('GGG'::kmer)
)
SELECT kmer, count(*) AS cnt
FROM kmers
GROUP BY kmer
ORDER BY cnt;
