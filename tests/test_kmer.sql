-- Input/output
SELECT 'ACGT'::kmer;
SELECT 'acgt'::kmer;
-- Length
SELECT length('AC'::kmer);
SELECT length('ACGTACGTACGTACGTACGTACGTACGTACGT'::kmer);
-- Errors
SELECT 'A' || repeat('C', 40)::kmer;
SELECT 'AXGT'::kmer;