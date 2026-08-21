# mini-db

A persistent, networked, benchmarked key-value store built from scratch in C++, created to understand how real databases work internally — not to replace existing ones, but to learn by building.

## Why this project

Every developer uses databases (MySQL, MongoDB, Redis) as black boxes. This project is my attempt to open that box — implementing the core mechanics myself: how data gets persisted to disk, why indexing matters, how deletes actually work under the hood, how databases recover from crashes, how concurrent access is made safe, how a database is exposed as a real network service, and how to measure and optimize real performance.

## Current features

- `SET key value` / `GET key` / `DELETE key` — full CRUD, available both as direct function calls and over the network
- In-memory indexing (`unordered_map<string, streampos>`) for fast key lookups, avoiding full file scans
- Tombstone-based deletes, with automatic background compaction reclaiming space from stale/deleted entries in both the data file and the write-ahead log
- Write-Ahead Logging (WAL) — append-only, with only the most recent entry replayed on crash recovery, since earlier entries are guaranteed already applied
- Thread-safe concurrent access via a readers-writer lock (`shared_mutex`)
- A TCP server exposing the database over the network, with each client handled concurrently on its own thread
- Benchmarking tools (`benchmarkSet`, `benchmarkGet`) measuring real throughput

## Performance

Optimized iteratively, measuring before and after each change:

| Stage | SET (ops/sec) | GET (ops/sec) |
|---|---|---|
| Baseline (file opened/closed per call) | ~2,200 | — |
| Persistent `data.db` write handle | ~4,100 | — |
| Append-only WAL, persistent handle | ~357,000 | — |
| `thread_local` persistent read handle | — | ~750,000+ |

The biggest wins came from eliminating repeated file open/close calls — each `open()`/`close()` involves real OS overhead, and the original design paid that cost on every single operation. Correctness was re-verified after each optimization, not just throughput.

## How it works

- Data is stored as plain text in `data.db`, one `key=value` pair per line, written append-only; the most recent line for a key is the current value ("last write wins")
- Deletes write a `__Deleted__` marker; actual removal happens during compaction
- The in-memory index is rebuilt from the file at startup and kept in sync incrementally on every write
- `data.db` and `wal.log` are each opened once, kept open persistently, and explicitly flushed after every write — avoiding repeated open/close overhead while preserving crash-safety guarantees
- Each thread reading via `get()` keeps its own persistently-open file handle (`thread_local`), avoiding open/close overhead without risking the race conditions that would come from sharing one read handle across concurrent readers
- The WAL is append-only; since each `set()` call's WAL write is guaranteed to complete before the next call begins (protected by the writer lock), only the last WAL entry can possibly be unapplied after a crash — everything before it is guaranteed already written to `data.db`
- A background thread periodically compacts both `data.db` and `wal.log`, so neither grows unbounded
- The server listens on a TCP socket; each client connection is handled on its own thread, with all database operations protected by the shared_mutex

## How to run

```bash
g++ main.cpp -o main -pthread
./main
```

Then, from another terminal, connect as a client:
```bash
nc localhost 9999
```
and send commands like:

SET username rahul123
GET username
DELETE username


## Design decisions & trade-offs

- **Persistent file handles over open-per-call**: dramatically faster, but requires explicit `.flush()` after every write to preserve the durability guarantees the WAL depends on
- **thread_local read handles over one shared handle**: sharing one `ifstream` across concurrent readers would create a race condition on the file's internal read position; giving each thread its own handle avoids this while still eliminating open/close overhead
- **Append-only WAL, single-entry replay**: much faster to write than the original open/write/clear/close pattern, and simpler to reason about correctness for, since replay only ever needs the last line
- **Readers-writer lock over per-key locking**: simpler to implement correctly; a per-key scheme would allow more parallelism at the cost of significant added complexity
- **Thread-per-client server model**: simple and correct at moderate load; a production system at very high concurrency might use a thread pool or async I/O instead

## Roadmap

- [x] In-memory indexing for fast lookups
- [x] Compaction for space reclamation (data file and WAL, both automatic)
- [x] Write-ahead logging for crash recovery
- [x] Thread-safe concurrent access (readers-writer lock)
- [x] TCP server with GET/SET/DELETE, concurrent client handling
- [x] Benchmarking and iterative performance optimization
- [ ] On-disk data structure upgrade (e.g. B-tree) to avoid full-file index rebuilds
- [ ] Server-level throughput benchmarking (through the full network path, not just direct function calls)
