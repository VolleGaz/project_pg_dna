pg_dna — PostgreSQL Extension for DNA / K-mer Analysis

INFO-H417 — Database System Architecture (2025–2026)

This repository contains a custom PostgreSQL extension implemented in C for DNA sequence manipulation, k-mer processing, and pattern matching.
It is designed as part of the INFO-H417 project (ULB) and executed inside a reproducible Docker environment.

Development Setup (Docker Compose + PostgreSQL 16)

All development and compilation occur inside a Dockerized PostgreSQL 16 server to ensure identical behavior for all team members.

1. Start PostgreSQL and pgAdmin using Docker Compose

From the project root:

docker compose up -d


This launches:

PostgreSQL 16 on port 5440

pgAdmin on port 8081

A bind-mounted project directory inside the container at /pg_dna

Verify that both containers are running:

docker ps


Expected containers:

pg_dna_dev

pgadmin_pg_dna

2. Enter the PostgreSQL container
docker exec -it pg_dna_dev bash


Verify the project is correctly mounted:

ls -l /pg_dna


Expected:

Makefile
src/
sql/
pg_dna.control
README.md


If the directory is empty, see Troubleshooting.

3. Install compilation tools (first-time only)

Inside the container:

apt-get update
apt-get install -y \
    build-essential \
    clang llvm-dev \
    libpq-dev \
    postgresql-server-dev-16


These provide:

gcc, make

PostgreSQL C development headers

pg_config

LLVM bitcode tools

4. Compile and install the extension

Inside the container:

cd /pg_dna
make clean
make
make install


After installation, restart PostgreSQL:

exit
docker restart pg_dna_dev
docker exec -it pg_dna_dev bash
psql -U postgres

5. Load and test the extension

Inside PostgreSQL:

CREATE EXTENSION pg_dna;
SELECT 'ACGTAC'::dna;
SELECT 'acgtacgt'::dna;
SELECT 'AXGT'::dna;


Expected results:

ACGTAC

ACGTACGT

Error: invalid base X

If these succeed, the extension is correctly installed.

Troubleshooting
Problem: /pg_dna is empty inside the container

Cause: The bind mount is not working.

Fix:

Ensure your docker-compose.yml includes:

volumes:
  - .:/pg_dna


Reset environment:

docker compose down
rm -rf data/
docker compose up -d


Ensure you can run Docker without sudo:

sudo usermod -aG docker $USER
newgrp docker

Problem: data/ cannot be removed

Cause: It was created by Docker under root.

Fix:

sudo rm -rf data/

Problem: PostgreSQL cannot load pg_dna.so

Cause: PostgreSQL must be restarted after each make install.

Fix:

docker restart pg_dna_dev

Problem: Missing PostgreSQL headers

If you see:

postgres.h: No such file or directory


Install:

apt-get install postgresql-server-dev-16

Project Structure
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

Team Usage (ULB)

Each team member can:

Clone the repository

Start the environment with docker compose up -d

Enter the container and compile the extension

Edit source files locally on their machine

Rebuild using make clean && make && make install

Restart PostgreSQL and test with psql

This ensures consistent development and avoids dependency mismatches.

Notes for Contributors

Always compile inside the container

Never copy .so files manually

Always restart PostgreSQL after installation

Keep Makefile portable

Avoid committing root-owned files

Use Git branches for new features
