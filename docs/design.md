# Detailed Design Specification (Weeks 1 – 12)

**Course**: KL University — 25CS2104E OS Capstone (Project 19)

---

## 1. Protocol & Command Grammar

```ebnf
line        := wsp* command wsp* ( args )* wsp* newline?
command     := "SET" | "GET" | "DEL" | "COUNT" | "EXPIRE" | "TTL" | "COMPACT" | "EXIT" | "QUIT"
args        := token ( wsp+ token )*
token       := bare | quoted
bare        := ( [^ \t\r\n\0"] )+
quoted      := '"' ( [^"\\] | '\"' | '\\' )* '"'
```

### Command Reference

| Command | Arguments | Semantics | Reply |
|:--------|:----------|:----------|:------|
| `SET` | `key value [ttl_ms]` | Stores `value` under `key`. Optional `ttl_ms` sets expiration in milliseconds. | `OK` or `ERROR` |
| `GET` | `key` | Retrieves value associated with `key`. If expired, triggers lazy eviction. | `"value"` or `(nil)` |
| `DEL` | `key` | Removes `key` and frees its memory. | `OK` or `NOT_FOUND` |
| `COUNT` | *none* | Returns total number of live keys. | `<integer>
` |
| `EXPIRE` | `key ttl_ms` | Sets time-to-live of `ttl_ms` milliseconds on existing `key`. | `OK` or `NOT_FOUND` |
| `TTL` | `key` | Returns remaining time-to-live in milliseconds. | Positive ms, `-1` (no TTL), or `-2` (missing/expired) |
| `COMPACT` | *none* | Triggers online snapshot compaction of the WAL. | `OK` or `ERROR` |
| `EXIT` / `QUIT` | *none* | Closes client connection. | *connection closed* |

---

## 2. WAL Framing & Persistence Format

Records in the append-only log are structured in network byte order:
1. `op` (1 byte): `0x01` (`WAL_OP_SET`), `0x02` (`WAL_OP_DEL`), `0x03` (`WAL_OP_SET_TTL`).
2. `key_len` (2 bytes, `uint16_t`): Length of key string.
3. `val_len` (4 bytes, `uint32_t`): Length of value string (0 for DEL).
4. `key` (`key_len` bytes): Raw key UTF-8/ASCII bytes.
5. `val` (`val_len` bytes): Raw value UTF-8/ASCII bytes.
6. `expire_at_ms` (8 bytes, `uint64_t`, only present if `op == 0x03`): Unix epoch milliseconds.
7. `crc32` (4 bytes, `uint32_t`): IEEE 802.3 checksum over fields 1 through 6.

---

## 3. Concurrency & Synchronization Model

```
KVStore Table:
[ Bucket 0 ]  ---> [ Stripe 0  RW-Lock ]
[ Bucket 1 ]  ---> [ Stripe 1  RW-Lock ]
...
[ Bucket 31]  ---> [ Stripe 31 RW-Lock ]
[ Bucket 32]  ---> [ Stripe 0  RW-Lock ] (modulo 32)
```

- **Lock Acquisition Ordering**:
  - Point queries: `lock_key_read(key)` or `lock_key_write(key)` on single stripe.
  - Full-table operations (table resizing, online compaction, lock mode switching): `lock_all_write(kv)` acquires all 32 locks strictly in ascending order ($0 \dots 31$) to prevent deadlocks.
- **ThreadSanitizer Invariant**:
  - Maximum simultaneously acquired locks per thread is 32, well within TSan's deadlock detector limit of 64 (`n_all_locks_ < 64`).

---

## 4. Memory Discipline & Ownership

- **Allocation Policy**: Every heap buffer allocated via `malloc`/`calloc`/`strdup` is tracked and freed.
- **Valgrind Guarantee**: Verified 0 bytes leaked in 0 blocks across all 10 unit and integration tests.
- **Destruction**: `kvstore_destroy(kv)` stops the background TTL reaper thread, flushes and closes the WAL descriptor, traverses all buckets freeing keys and values, destroys all 32 `pthread_rwlock_t` instances and the global mutex, and frees the `KVStore` container.
