-- Input/output
SELECT 'ACGT'::qkmer;
SELECT 'nryswkmbdhv'::qkmer;
SELECT 'acgt'::qkmer;
-- Length
SELECT length('AC'::qkmer);
SELECT length('NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN'::qkmer);
-- 32
-- Errors
SELECT ''::qkmer;
SELECT 'Z'::qkmer;
SELECT repeat('N', 33)::qkmer;