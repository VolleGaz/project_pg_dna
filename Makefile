MODULE_big = pg_dna
OBJS = src/pg_dna.o src/dna.o src/kmer.o src/qkmer.o src/funcs.o src/ops_kmer.o src/hash_btree_kmer.o src/spgist_kmer.o

EXTENSION = pg_dna
DATA = sql/pg_dna--1.0.sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

test:
	psql -v ON_ERROR_STOP=1 -U postgres -f tests/test_dna.sql
	psql -v ON_ERROR_STOP=1 -U postgres -f tests/test_generate_kmers.sql
	psql -v ON_ERROR_STOP=1 -U postgres -f tests/test_kmer.sql
	psql -v ON_ERROR_STOP=1 -U postgres -f tests/test_qkmer.sql
