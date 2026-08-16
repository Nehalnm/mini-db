# mini-db

A persistent key-value store built from scratch in C++, created to understand how real databases work internally — not to replace existing ones, but to learn by building.

## Why this project

Every developer uses databases (MySQL, MongoDB, Redis) as black boxes. This project is my attempt to open that box — implementing the core mechanics myself: how data gets persisted to disk, why indexing matters, how deletes actually work under the hood, and the trade-offs involved in each design decision.

## Current features

- `set(key, value)` — store a key-value pair, persisted to disk
- `get(key)` — retrieve the most recent value for a key
- `deleteKey(key)` — remove a key using a tombstone marker
- In-memory indexing (`unordered_map`) for fast key lookups, built at startup by scanning the data file

## How it works

- Data is stored as plain text in `data.db`, one `key=value` pair per line
- Writes are append-only — updates and deletes don't modify existing lines, they add new ones
- On lookup, the most recent line for a given key is treated as the current value ("last write wins")
- Deletes write a special `__Deleted__` marker instead of removing data — actual removal happens later via compaction (not yet implemented)
- An in-memory index maps each key to its most recent byte position in the file, avoiding a full file scan on every lookup

## How to run

```bash
g++ main.cpp -o main
./main
```

## Design decisions & trade-offs

- **Append-only writes**: fast, but means the file grows indefinitely without periodic cleanup (compaction)
- **Tombstone deletes**: simple and consistent with the existing format, but doesn't reclaim disk space immediately
- **In-memory index**: makes lookups fast, but is rebuilt from scratch every time the program starts, since it isn't persisted

## Roadmap

- [ ] Rewrite `get()` to use the index (`seekg`) instead of scanning the file
- [ ] Compaction — reclaim space from stale/deleted entries
- [ ] Write-ahead logging for crash recovery
- [ ] Concurrency support (multiple readers/writers)
- [ ] TCP server interface for remote access
- [ ] Benchmarking and performance optimization
