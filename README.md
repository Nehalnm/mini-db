# mini-db

A persistent key-value store built from scratch in C++, created to understand how real databases work internally — not to replace existing ones, but to learn by building.

## Why this project

Every developer uses databases (MySQL, MongoDB, Redis) as black boxes. This project is my attempt to open that box — implementing the core mechanics myself: how data gets persisted to disk, why indexing matters, how deletes actually work under the hood, how databases recover from crashes, and the trade-offs involved in each design decision.

## Current features

- `set(key, value)` — store a key-value pair, persisted to disk
- `get(key)` — retrieve the most recent value for a key, using an in-memory index for fast lookups
- `deleteKey(key)` — remove a key using a tombstone marker
- `compact()` — reclaim disk space by rewriting the data file with only the latest, non-deleted values
- Write-Ahead Logging (WAL) — operations are logged before being applied, so the database can recover from a crash mid-write without corruption or data loss

## How it works

- Data is stored as plain text in `data.db`, one `key=value` pair per line
- Writes are append-only — updates and deletes don't modify existing lines, they add new ones
- On lookup, the most recent line for a given key is treated as the current value ("last write wins")
- Deletes write a special `__Deleted__` marker instead of removing data — actual removal happens during compaction
- An in-memory index (`unordered_map<string, streampos>`) maps each key to its most recent byte position in the file, avoiding a full file scan on every lookup. It's rebuilt from the file at startup and kept in sync incrementally on every write
- Compaction rewrites `data.db` from scratch using the index as the source of truth, discarding stale and deleted entries, then rebuilds the index against the clean file
- Every `set()` call first writes its intent to `wal.log`, then applies the change to `data.db` and the index, then clears the log. On startup, if `wal.log` has leftover content, it means the program crashed mid-operation — that entry is replayed automatically before normal operation resumes

## How to run

g++ main.cpp -o main
./main

## Design decisions & trade-offs

- **Append-only writes**: fast, but means the file grows indefinitely without periodic compaction
- **Tombstone deletes**: simple and consistent with the existing format, but doesn't reclaim disk space until compaction runs
- **In-memory index**: makes lookups fast, but is rebuilt from a full file scan every time the program starts, since it isn't itself persisted
- **Single-entry WAL**: the log only ever holds one pending operation at a time, kept simple for a single-writer program; a concurrent version would need a proper append-only log with per-entry tracking
- **Text-based log format**: simple to implement and debug, but doesn't support keys/values containing spaces — a known limitation, would need escaping or length-prefixing to fix properly

## Roadmap

- [x] Rewrite get() to use the index instead of scanning the file
- [x] Compaction — reclaim space from stale/deleted entries
- [x] Write-ahead logging for crash recovery
- [ ] Concurrency support (multiple readers/writers)
- [ ] TCP server interface for remote access
- [ ] Benchmarking and performance optimization
