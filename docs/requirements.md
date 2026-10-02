cat > docs/requirements.md << 'EOF'
# Requirements

## 1. Functional Requirements
- FR1: SET key value — insert or overwrite a key-value pair.
- FR2: GET key — retrieve the value for a key, or report missing.
- FR3: DEL key — remove a key; report whether it existed.
- FR4: REPL interface for interactive testing (Week 1).
- FR5: TCP line/RESP-style protocol for remote clients (Week 4).
- FR6: Durability via append-only log with startup replay (Week 7).
- FR7: Log compaction to bound log growth (Week 9).
- FR8: TTL / key expiry (Week 11).

## 2. Non-Functional Requirements
- NFR1: O(1) average-case lookup, insert, delete.
- NFR2: Thread-safe concurrent access (reader/writer or striped locks).
- NFR3: Crash-consistent — state survives kill -9 and restart.
- NFR4: Valgrind-clean — no memory or descriptor leaks under churn.
- NFR5: TSan-clean under concurrent soak.

## 3. Input / Output Requirements
- Input: line-based ASCII commands (`SET k v`, `GET k`, `DEL k`).
- Output: plain-text replies (`OK`, `NOT_FOUND`, `"value"`, `(nil)`).
- Protocol: must tolerate partial reads and pipelined requests.

## 4. Hardware / Software Requirements
- OS: Ubuntu 24.04 LTS on WSL2
- Compiler: gcc with C11 (`-std=c11`)
- Build: GNU Make
- Version control: Git + GitHub
- Analysis tools: Valgrind, ThreadSanitizer, gdb, strace
- Libraries: pthreads, POSIX sockets (no external deps)

## 5. Constraints
- No external database or third-party hash-table library — the index is
  implemented from scratch.
EOF
cat docs/requirements.md
