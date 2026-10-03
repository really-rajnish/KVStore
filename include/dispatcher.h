#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <stdbool.h>
#include <stdio.h>

#include "kvstore.h"
#include "parser.h"

/* Execute `cmd` against `kv`, writing any reply to `out`.
   Returns true to continue, false to signal shutdown (EXIT/QUIT). */
bool dispatch_command(KVStore *kv, const ParsedCommand *cmd, FILE *out);

#endif /* DISPATCHER_H */
