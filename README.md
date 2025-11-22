# pg_dna — PostgreSQL Extension for DNA / K-mer Analysis  
INFO-H417 — Database System Architecture (2025–2026)

This repository contains a custom PostgreSQL extension implemented in C for DNA sequence manipulation, k-mer processing, and pattern matching.  
It is designed as part of the INFO-H417 project (ULB) and executed inside a reproducible Docker environment.

---

Ensure Docker runs without sudo:

```bash
sudo usermod -aG docker $USER
newgrp docker
```
---

# Development Setup (Docker Compose + PostgreSQL 16)

All development and compilation occur inside a Dockerized PostgreSQL 16 server to ensure identical behavior for all team members.

---

## 1. Start PostgreSQL and pgAdmin using Docker Compose

From the project root:

```bash
docker compose up -d
```

This launches:

- PostgreSQL 16 on port 5440  
- pgAdmin on port 8081  
- A bind-mounted project directory inside the container at `/pg_dna`

Verify that both containers are running:

```bash
docker ps
```

Expected containers:

- `pg_dna_dev`
- `pgadmin_pg_dna`

---

## 2. Enter the PostgreSQL container

```bash
docker exec -it pg_dna_dev bash
```

Verify that the project is visible:

```bash
ls -l /pg_dna
```

Expected:

```
Makefile
src/
sql/
pg_dna.control
README.md
```

If this directory is empty, see Troubleshooting.

---

## 3. Install compilation tools (first-time only)

Inside the container:

```bash
apt-get update
apt-get install -y     build-essential     clang llvm-dev     libpq-dev     postgresql-server-dev-16
```

These provide:

- gcc, make  
- PostgreSQL C development headers  
- pg_config  
- LLVM bitcode tools  

---

## 4. Compile and install the extension

Inside the container:

```bash
cd /pg_dna
make clean
make
make install
```

Restart PostgreSQL so it loads the new `.so`:

```bash
exit
docker restart pg_dna_dev
docker exec -it pg_dna_dev bash
psql -U postgres
```

---

## 5. Load and test the extension

Inside PostgreSQL:

```sql
CREATE EXTENSION pg_dna;
SELECT 'ACGTAC'::dna;
SELECT 'acgtacgt'::dna;
SELECT 'AXGT'::dna;
```

Expected results:

- `ACGTAC`  
- `ACGTACGT`  
- Error: invalid base `X`  

If this behaves correctly, the extension is properly installed.

---

# Troubleshooting

## `/pg_dna` is empty inside the container

Cause: The bind mount failed.

Fix:

1. Ensure your `docker-compose.yml` includes:

```
volumes:
  - .:/pg_dna
```

2. Reset environment:

```bash
docker compose down
rm -rf data/
docker compose up -d
```

3. Ensure Docker runs without sudo:

```bash
sudo usermod -aG docker $USER
newgrp docker
```

---

## `data/` cannot be removed

Cause: It was created using sudo.

Fix:

```bash
sudo rm -rf data/
```

---

## PostgreSQL cannot load `pg_dna.so`

Cause: PostgreSQL must be restarted after each installation.

Fix:

```bash
docker restart pg_dna_dev
```

---

## Missing headers (`postgres.h: No such file or directory`)

Fix:

```bash
apt-get install postgresql-server-dev-16
```

---

# Project Structure

```
project_pg_dna/
│
├── docker-compose.yml
├── Makefile
├── pg_dna.control
├── sql/
│   └── pg_dna--1.0.sql
├── src/
│   ├── dna.c
│   ├── kmer.c
│   ├── qkmer.c
│   ├── funcs.c
│   ├── ops_kmer.c
│   ├── hash_btree_kmer.c
│   ├── spgist_kmer.c
│   └── ...
└── README.md
```

---

# Team Usage (ULB)

Each team member can:

1. Clone the repository  
2. Start the Docker environment with `docker compose up -d`  
3. Enter the container and compile the extension  
4. Edit the code locally from the host machine  
5. Rebuild using `make clean && make && make install`  
6. Restart PostgreSQL and test  

This ensures a consistent development environment.

---

# Notes for Contributors

- Always compile inside Docker  
- Never copy `.so` files manually  
- Always restart PostgreSQL after installation  
- Keep the Makefile clean and portable  
- Avoid committing root-owned files  
- Use Git branches for new features  
