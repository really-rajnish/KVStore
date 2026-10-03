# Testing

## 1. Strategy

Unit tests, one binary per module. Every binary prints PASS / FAIL per
assertion and exits non-zero if anything failed, so make test can chain
them with || exit 1.

## 2. Test Binaries

| Binary                 | Source                    | Covers                            |
|------------------------|---------------------------|-----------------------------------|
| bin/test_hash_table    | tests/test_hash_table.c   | ht_create/set/get/del/resize      |
| bin/test_kvstore       | tests/test_kvstore.c      | KVStore wrapper behaviour         |
| bin/test_parser        | tests/test_parser.c       | tokeniser, quotes, escapes, limits|

## 3. Coverage per Module

### hash_table
- create / destroy
- insert, lookup, overwrite, delete
- delete of a missing key returns false
- resize under 500 inserts (load factor path)
- lookup still correct after resize

### kvstore
- set / get / del round-trip
- overwrite does not change size
- delete of a missing key returns false

### parser
- simple SET k v tokenisation
- quoted token keeps interior spaces
- escaped \" and \\ decoded
- GET / DEL / COUNT / EXIT / unknown commands
- tab + multiple-space whitespace
- empty input and whitespace-only input rejected
- unterminated quote rejected

## 4. Memory Safety

Every binary is run under Valgrind before a commit:

    printf 'SET a 1\nGET a\nDEL a\nEXIT\n' | \
        valgrind --leak-check=full --error-exitcode=1 ./bin/kvstore

Acceptance: in use at exit: 0 bytes in 0 blocks, ERROR SUMMARY: 0 errors.

## 5. Running the Suite

    make test

Expected tail: ALL TESTS PASSED (0 failures) for each binary.

## 6. Regression Discipline

Any change to parser.c or hash_table.c must be followed by make test and
a fresh Valgrind run before the commit.
