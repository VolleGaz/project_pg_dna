DROP EXTENSION IF EXISTS pg_dna CASCADE;
CREATE EXTENSION pg_dna;

SET enable_seqscan = on;
SET enable_indexscan = off;
SET enable_bitmapscan = off;

DROP TABLE IF EXISTS test_kmers;
CREATE TABLE test_kmers (
    id          serial PRIMARY KEY,
    kmer_value  kmer NOT NULL
);

\echo 'insertion de 5 000 000 kmers random (len=10)'

INSERT INTO test_kmers (kmer_value)
SELECT (
    (ARRAY['A','C','G','T'])[1 + floor(random() * 4)::int] ||
    (ARRAY['A','C','G','T'])[1 + floor(random() * 4)::int] ||
    (ARRAY['A','C','G','T'])[1 + floor(random() * 4)::int] ||
    (ARRAY['A','C','G','T'])[1 + floor(random() * 4)::int] ||
    (ARRAY['A','C','G','T'])[1 + floor(random() * 4)::int] ||
    (ARRAY['A','C','G','T'])[1 + floor(random() * 4)::int] ||
    (ARRAY['A','C','G','T'])[1 + floor(random() * 4)::int] ||
    (ARRAY['A','C','G','T'])[1 + floor(random() * 4)::int] ||
    (ARRAY['A','C','G','T'])[1 + floor(random() * 4)::int] ||
    (ARRAY['A','C','G','T'])[1 + floor(random() * 4)::int]
)::kmer
FROM generate_series(1, 5000000);

\echo 'creation index spgist sur kmer_value...'

DROP INDEX IF EXISTS idx_kmer_spgist;
CREATE INDEX idx_kmer_spgist ON test_kmers USING spgist (kmer_value);

ANALYZE test_kmers;


\echo '==== test 1 : egalite (=) ultra selective ===='
\echo 'valeur testee : AAAAAAAAAA'

SET enable_seqscan = on;
SET enable_indexscan = off;
SET enable_bitmapscan = off;

\echo '--- test 1A : = avec seqscan only ---'
EXPLAIN ANALYZE
SELECT *
FROM test_kmers
WHERE kmer_value = 'AAAAAAAAAA'::kmer;

SET enable_seqscan = off;
SET enable_indexscan = on;
SET enable_bitmapscan = off;

\echo '--- test 1B : = avec index spgist only ---'
EXPLAIN ANALYZE
SELECT *
FROM test_kmers
WHERE kmer_value = 'AAAAAAAAAA'::kmer;


\echo '==== test 2 : prefix (^@) tres selectif ===='
\echo 'prefix utilise : AAAAAAAC (8 chars)'

SET enable_seqscan = on;
SET enable_indexscan = off;
SET enable_bitmapscan = off;

\echo '--- test 2A : ^@ avec seqscan only ---'
EXPLAIN ANALYZE
SELECT *
FROM test_kmers
WHERE kmer_value ^@ 'AAAAAAAC'::kmer;

SET enable_seqscan = off;
SET enable_indexscan = on;
SET enable_bitmapscan = off;

\echo '--- test 2B : ^@ avec index spgist only ---'
EXPLAIN ANALYZE
SELECT *
FROM test_kmers
WHERE kmer_value ^@ 'AAAAAAAC'::kmer;


\echo '==== test 3 : pattern (<@) tres selectif ===='
\echo 'pattern utilise : AAAA AAACNN (qkmer)'

SET enable_seqscan = on;
SET enable_indexscan = off;
SET enable_bitmapscan = off;

\echo '--- test 3A : <@ avec seqscan only ---'
EXPLAIN ANALYZE
SELECT *
FROM test_kmers
WHERE kmer_value <@ 'AAAAAAACNN'::qkmer;

SET enable_seqscan = off;
SET enable_indexscan = on;
SET enable_bitmapscan = off;

\echo '--- test 3B : <@ avec index spgist only ---'
EXPLAIN ANALYZE
SELECT *
FROM test_kmers
WHERE kmer_value <@ 'AAAAAAACNN'::qkmer;


RESET enable_seqscan;
RESET enable_indexscan;
RESET enable_bitmapscan;


\echo '-- fin du test spgist'

