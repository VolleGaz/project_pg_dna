# pg_dna submission bundle

This folder collects the minimal artefacts requested by the assignment: a runnable SQL script with sample plans/results, and brief build/run notes.

## Environment
- PostgreSQL: 16 (container `pg_dna_dev` in `docker-compose.yml`)
- Extension version: 1.0 (`pg_dna--1.0.sql`)

## Build + install (inside the container)
```bash
docker exec -it pg_dna_dev bash -lc "cd /pg_dna && make clean && make && make install"
docker restart pg_dna_dev   # reloads the shared library
```

## Run the submission test script
```bash
docker exec -it pg_dna_dev bash -lc "cd /pg_dna && psql -v ON_ERROR_STOP=1 -U postgres -f submission/test.sql" > submission/test_output.txt
```
This exercises:
- Type I/O and length for `dna`, `kmer`, `qkmer`
- Aliases/operators: `equals`, `starts_with`, `contains`, `=`, `^@`, `@>`, `<@`
- `generate_kmers` SRF
- `GROUP BY`/`ORDER BY` on kmers (hash/btree opclasses)

`submission/test_output.txt` is the captured output.

## Optional SP-GiST demo
The heavier SP-GiST index performance script lives at `tests/test_spgist.sql`. Run it separately if you want plan/perf evidence:
```bash
docker exec -it pg_dna_dev bash -lc "cd /pg_dna && psql -v ON_ERROR_STOP=1 -U postgres -f tests/test_spgist.sql"
```
