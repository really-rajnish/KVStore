# Design

## 1. Modules

    parser      line  -> ParsedCommand { type, argc, argv[] }
    dispatcher  ParsedCommand + KVStore -> reply on FILE* out
    kvstore     get / set / del / size  (wraps the hash table)
    hash_table  O(1) average index (chaining, djb2)

The pipeline is deliberately protocol-agnostic. In Week 1 the REPL feeds
parse_line a buffer from stdin; in Week 4 the server will feed it a buffer
from a socket. Neither parser nor dispatcher changes.

## 2. Command Grammar (Week 3)

    line        := wsp* command wsp* args? wsp* newline?
    command     := "SET" | "GET" | "DEL" | "COUNT" | "EXIT" | "QUIT"
    args        := token ( wsp+ token )*
    token       := bare | quoted
    bare        := ( any char except space / tab / NUL )+
    quoted      := '"' ( any char | '\"' | '\\' )* '"'

    Semantics:
      SET   key value   ->  stores value under key
      GET   key         ->  returns value or (nil)
      DEL   key         ->  OK if removed, NOT_FOUND otherwise
      COUNT             ->  number of live keys
      EXIT | QUIT       ->  terminates the session

Limits (compile-time):
    PARSER_MAX_ARGS  = 8     max tokens per line
    PARSER_MAX_TOKEN = 256   max characters per token

## 3. Log-Record Layout (Week 2, forward-looking)

    record := op(1) keylen(2) vallen(4) key(keylen) value(vallen) crc32(4)

    op      : 0x01 = SET, 0x02 = DEL
    keylen  : uint16, network byte order
    vallen  : uint32, network byte order (0 for DEL)
    crc32   : over the preceding bytes; detects torn writes

## 4. Memory Ownership (Week 2)

| Owner         | Object                       | Freed by                          |
|---------------|------------------------------|-----------------------------------|
| Caller        | char *line passed to parse   | Caller's stack                    |
| ParsedCommand | storage[][], argv[]          | Automatic (stack); no free needed |
| HashTable     | buckets + HTEntry keys/values| ht_destroy                        |
| KVStore       | the HashTable pointer        | kvstore_destroy                   |
| ht_get return | borrows from the table       | NOT freed by the caller           |

Rules:
  - Every malloc has exactly one free.
  - ht_set duplicates key and value; caller keeps ownership of originals.
  - ht_get / kvstore_get return a borrowed pointer valid until the next
    mutation of that key. Never call free on it.
  - Valgrind must report zero leaks after every test binary.

## 5. Data Flow (Week 3 REPL path)

    stdin  ->  fgets            (main.c)
           ->  parse_line       (parser.c)
           ->  dispatch_command (dispatcher.c)
           ->  kvstore_set/get/del (kvstore.c)
           ->  ht_set/get/del   (hash_table.c)
           ->  response on stdout

## 6. Design Decisions

- Parser copies tokens into ParsedCommand storage; the input buffer is
  never modified. Safe to reuse on socket buffers that must be preserved
  for partial-read handling (Week 4).
- Command table by strcmp — linear scan over 6 strings is not a hot path.
- Dispatcher writes to a FILE* — the REPL passes stdout, the server will
  pass fdopen(sockfd, "w"). Same code, no fork needed.
- Quoted strings with escapes — lets clients store values containing
  spaces without a length-prefixed protocol.
