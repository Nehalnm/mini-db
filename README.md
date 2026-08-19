# mini-db

A persistent, networked key-value store built from scratch in C++, created to understand how real databases work internally — not to replace existing ones, but to learn by building.

## Why this project

Every developer uses databases (MySQL, MongoDB, Redis) as black boxes. This project is my attempt to open that box — implementing the core mechanics myself: how data gets persisted to disk, why indexing matters, how deletes actually work under the hood, how databases recover from crashes, how concurrent access is made safe, and how a database is exposed as a real network service.

## Current features

- `SET key value` / `GET key` / `DELETE key` — full CRUD, available both as direct function calls and over the network
- In-memory indexing (`unordered_map<string, streampos>`) for fast key lookups, avoiding full file scans
- Tombstone-based deletes, with `compact()` reclaiming space by rewriting the data file with only the latest, non-deleted values
- Automatic background compaction — runs periodically on its own thread, no manual/client trigger needed
- Write-Ahead Logging (WAL) — operations are logged before being applied, so the database recovers correctly from a crash mid-write
- Thread-safe concurrent access via a readers-writer lock (`shared_mutex`) — multiple readers can proceed simultaneously, writers get exclusive access
- A TCP server exposing the database over the network — multiple clients can connect and issue commands concurrently, each handled on its own thread

## How it works

- Data is stored as plain text in `data.db`, one `key=value` pair per line, written append-only
- The most recent line for a given key is treated as the current value ("last write wins")
- Deletes write a `__Deleted__` marker instead of removing data immediately; actual removal happens during compaction
- The in-memory index is rebuilt from the file at startup and kept in sync incrementally on every write
- Every write first logs its intent to `wal.log`, then applies the change, then clears the log — if the log has leftover content at startup, it's replayed automatically
- The server listens on a TCP socket; each client connection is handled on a separate thread, with all database operations protected by a shared_mutex for safe concurrent access
- A background thread periodically runs compaction, so the data file doesn't grow indefinitely, without needing any client involvement

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

- **Append-only writes**: fast, but requires periodic compaction to reclaim space — handled automatically in the background
- **Tombstone deletes**: simple and consistent with the existing format, doesn't reclaim space until compaction runs
- **In-memory index**: fast lookups, but rebuilt via a full file scan on every startup, since it isn't itself persisted
- **Single-entry WAL, text-based format**: simple to implement and reason about, but doesn't support keys/values containing spaces, and only tracks one pending operation at a time
- **Readers-writer lock over per-key locking**: simpler to implement correctly and reason about; a per-key locking scheme would allow more parallelism but adds significant complexity
- **Thread-per-client model**: simple and correct for moderate load; a production system at very high concurrency might use a thread pool or async I/O instead of spawning a new OS thread per connection

## Roadmap

- [x] In-memory indexing for fast lookups
- [x] Compaction for space reclamation (now automatic, background)
- [x] Write-ahead logging for crash recovery
- [x] Thread-safe concurrent access (readers-writer lock)
- [x] TCP server with GET/SET/DELETE over the network, concurrent client handling
- [ ] Benchmarking — measure real throughput/latency under load
- [ ] On-disk data structure upgrade (e.g. B-tree) to avoid full-file index rebuilds
