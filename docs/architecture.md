cat > docs/architecture.md << 'EOF'
# Architecture

## 1. High-Level Block Diagram

    +-------------+      +-------------+
    |   REPL      |      |  TCP Client |
    |  (Week 1)   |      |  (Week 4+)  |
    +------+------+      +------+------+
           |                    |
           |                    v
           |             +-------------+
           |             |   Server    |  accept loop / thread pool
           |             +------+------+
           |                    |
           +--------+-----------+
                    v
             +-------------+
             |   Parser    |  line -> {cmd, key, value}
             +------+------+
                    v
             +-------------+
             | Dispatcher  |  routes command to store operation
             +------+------+
                    v
             +-------------+
             |   KVStore   |  public API: set / get / del
             +------+------+
                    v
             +-------------+
             | Hash Table  |  O(1) average index
             +------+------+
                    v
             +-------------+
             | Append Log  |  durability (Week 7)
             +-------------+

## 2. Module Responsibilities

| Module        | File                | Responsibility                              |
|---------------|---------------------|---------------------------------------------|
| Hash Table    | src/hash_table.c    | Bucket array + chaining; djb2 hash; resize  |
| KVStore       | src/kvstore.c       | Wraps hash table; future log hook point     |
| Parser        | src/parser.c        | Tokenises a line into command + arguments   |
| Dispatcher    | src/dispatcher.c    | Maps parsed command to KVStore call         |
| Server        | src/server.c        | Socket accept, per-connection handling      |
| Client        | src/client.c        | Connects, sends commands, prints replies    |
| REPL          | src/main.c          | Week 1 interactive driver                   |

## 3. Data Flow (Week 1 REPL path)

    stdin line
      -> main.c reads with fgets
      -> sscanf splits into cmd / key / value
      -> kvstore_set / kvstore_get / kvstore_del
      -> ht_set / ht_get / ht_del
      -> result printed to stdout

## 4. Core Data Structure

    HashTable
      buckets  : array of HTEntry sentinel heads (chaining)
      capacity : number of buckets (power-of-two growth)
      size     : number of live keys

    HTEntry
      key   : heap-allocated string
      value : heap-allocated string
      next  : collision chain link

## 5. Design Decisions

- **Chaining over open addressing** — simpler deletion, no tombstone logic.
- **djb2 hash** — fast, well-distributed, trivial to defend in viva.
- **Load factor 0.75, double on growth** — keeps average chain length ~1.
- **Sentinel head nodes** — deletion does not need a special first-element case.
- **KVStore wrapper** — isolates the log/snapshot layer from the index so
  Week 7 can add durability without touching the hash table.

## 6. Technologies / Tools
- C11, gcc, GNU Make
- Git + GitHub
- Valgrind, ThreadSanitizer, gdb
EOF
cat docs/architecture.md
