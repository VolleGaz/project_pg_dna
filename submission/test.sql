-- Consolidated test script for pg_dna (assignment submission)
-- PostgreSQL 16 inside container pg_dna_dev

SET client_min_messages = WARNING;

DROP EXTENSION IF EXISTS pg_dna CASCADE;
CREATE EXTENSION pg_dna;

\echo '--- dna I/O and length ---'
SELECT 'ACGT'::dna AS dna_value, 'ACGT'::dna::text AS out_value;
SELECT length('AACCGGTT'::dna);

\echo '--- kmer I/O, length, aliases ---'
SELECT 'ACGT'::kmer AS kmer_value, 'acgt'::kmer AS kmer_lower;
SELECT length('AC'::kmer) AS len_short, length('ACGTACGTACGTACGTACGTACGTACGTACGT'::kmer) AS len_max;
SELECT equals('ACGT'::kmer, 'ACGT'::kmer) AS eq_true,
       equals('ACGT'::kmer, 'TGCA'::kmer) AS eq_false,
       starts_with('AC'::kmer, 'ACGT'::kmer) AS prefix_true,
       starts_with('GT'::kmer, 'ACGT'::kmer) AS prefix_false;

\echo '--- qkmer I/O, length, contains alias ---'
SELECT 'nryswkmbdhv'::qkmer AS qkmer_codes;
SELECT length('NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN'::qkmer) AS qlen_max;
SELECT contains('AN'::qkmer, 'AC'::kmer) AS contains_true,
       contains('AN'::qkmer, 'GT'::kmer) AS contains_false;

\echo '--- generate_kmers SRF ---'
SELECT array_agg(kmer::text ORDER BY kmer) AS kmers
FROM generate_kmers('ACGTAC'::dna, 3) AS k(kmer);

\echo '--- GROUP BY / counts ---'
WITH kmers AS (
    SELECT k.kmer, count(*) AS cnt
    FROM generate_kmers('ACGTACGT'::dna, 3) AS k(kmer)
    GROUP BY k.kmer
)
SELECT sum(cnt) AS total_count,
       count(*) AS distinct_count,
       count(*) FILTER (WHERE cnt = 1) AS unique_count
FROM kmers;

\echo '--- ORDER BY (btree ops) ---'
SELECT kmer::text AS kmer
FROM (VALUES ('ACG'::kmer), ('ACGTA'), ('ACGTC'), ('ACGTT'), ('ACT')) AS v(kmer)
ORDER BY kmer ASC;

\echo '--- Done ---'
