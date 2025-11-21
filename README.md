# pg_dna — PostgreSQL Extension for DNA / K-mer Analysis  
INFO-H417 — Database System Architecture (2025–2026)

This repository contains a custom PostgreSQL extension implemented in C for DNA sequence manipulation, k-mer processing, and pattern matching (qkmer with IUPAC ambiguous codes).  
It is designed as part of the INFO-H417 project (ULB).

---

#  Development Setup (Docker + PostgreSQL 16)

This project is designed to be compiled and executed **inside a Dockerized PostgreSQL environment**.

## 1. Start PostgreSQL 16 in Docker

```bash
docker run --name pg_dna_dev \
    -e POSTGRES_PASSWORD=postgres \
    -p 5432:5432 \
    -v /absolute/path/to/project_pg_dna:/pg_dna \
    -d postgres:16
```

 **IMPORTANT:**  
The path _must be absolute_. Example:

```
/home/dabel/Documents/project_pg_dna
```

If the container starts but `/pg_dna` is EMPTY inside Docker, you used the wrong path or Docker cannot access the folder.

---

## 2. Enter the container

```bash
docker exec -it pg_dna_dev bash
```

---

## 3. Install compilation tools

```bash
apt update
apt install -y build-essential postgresql-server-dev-16
```

These provide:

- `gcc`
- `make`
- PostgreSQL headers
- `pg_config`

---

## 4. Compile and install the extension

Inside the container:

```bash
cd /pg_dna
make clean
make
make install
```

---

## 5. Load the extension in PostgreSQL

```bash
psql -U postgres
```

Then:

```sql
DROP EXTENSION IF EXISTS pg_dna CASCADE;
CREATE EXTENSION pg_dna;
SELECT hello_dna();
```

You should see:

```
pg_dna extension loaded
```

---

#  Troubleshooting

##  Problem: `/pg_dna` appears empty inside Docker  
Cause: Docker cannot access the folder.

Solutions:

1. Ensure the absolute path is correct  
2. Ensure the folder exists  
3. Ensure Docker can traverse your home directory:

```bash
sudo chmod o+x /home/yourusername
```

---

##  Problem: version mismatch  
```
Server is version 16, library is version 14
```

Cause: `.so` was compiled on a different machine / different PostgreSQL version.

Fix:

```bash
make clean
make
make install
```

---

##  Problem: implicit declaration of cstring_to_text  
PostgreSQL 16 removed some older macros.

Use:

```c
#include "utils/builtins.h"
text *txt = cstring_to_text("...");
```

---

#  Project Structure

```
pg_dna/
│
├── src/
│   ├── pg_dna.c
│   ├── dna.c
│   ├── kmer.c
│   ├── qkmer.c
│   └── ...
│
├── sql/
│   └── pg_dna--1.0.sql
│
├── pg_dna.control
├── Makefile
└── README.md
```

---

#  Team Usage (for ULB group)

All team members can:

1. Clone this repository
2. Start the same Docker container
3. Compile the extension inside it
4. Modify C code from their host machine

```
git clone https://github.com/<team>/pg_dna_extension.git
```

---

#  Notes for Students

- Always recompile inside Docker  
- Never copy `.so` files manually  
- Use Git branches for major changes  
- Keep the Makefile simple and portable  

---

# 🧬 Authors
Group #X — INFO-H417 — ULB  
2025–2026
