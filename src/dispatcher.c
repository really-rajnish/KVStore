#include "dispatcher.h"
#include <stdlib.h>

static bool check_argc(const ParsedCommand *cmd, size_t want, FILE *out)
{
    size_t got = cmd->argc > 0 ? cmd->argc - 1 : 0;
    if (got != want) {
        fprintf(out, "ERROR: %s expects %zu argument%s, got %zu\n",
                command_type_name(cmd->type),
                want, want == 1 ? "" : "s", got);
        return false;
    }
    return true;
}

bool dispatch_command(KVStore *kv, const ParsedCommand *cmd, FILE *out)
{
    if (!kv || !cmd || !out) return false;

    switch (cmd->type) {

    case CMD_SET:
        if (cmd->argc == 3) {
            if (kvstore_set(kv, cmd->argv[1], cmd->argv[2]))
                fputs("OK\n", out);
            else
                fputs("ERROR: out of memory\n", out);
            return true;
        } else if (cmd->argc == 4) {
            int64_t ttl_ms = atoll(cmd->argv[3]);
            if (ttl_ms <= 0) {
                fputs("ERROR: invalid ttl\n", out);
                return true;
            }
            if (kvstore_set_ttl(kv, cmd->argv[1], cmd->argv[2], ttl_ms))
                fputs("OK\n", out);
            else
                fputs("ERROR: out of memory\n", out);
            return true;
        } else {
            fprintf(out, "ERROR: SET expects 2 or 3 arguments, got %zu\n",
                    cmd->argc > 0 ? cmd->argc - 1 : 0);
            return true;
        }

    case CMD_GET: {
        if (!check_argc(cmd, 1, out)) return true;
        char val_buf[4096];
        if (kvstore_get_copy(kv, cmd->argv[1], val_buf, sizeof(val_buf)))
            fprintf(out, "\"%s\"\n", val_buf);
        else
            fputs("(nil)\n", out);
        return true;
    }

    case CMD_DEL:
        if (!check_argc(cmd, 1, out)) return true;
        fputs(kvstore_del(kv, cmd->argv[1]) ? "OK\n" : "NOT_FOUND\n", out);
        return true;

    case CMD_COUNT:
        if (!check_argc(cmd, 0, out)) return true;
        fprintf(out, "%zu\n", kvstore_size(kv));
        return true;

    case CMD_EXPIRE: {
        if (!check_argc(cmd, 2, out)) return true;
        int64_t ttl_ms = atoll(cmd->argv[2]);
        if (ttl_ms <= 0) {
            fputs("ERROR: invalid ttl\n", out);
            return true;
        }
        if (kvstore_expire(kv, cmd->argv[1], ttl_ms))
            fputs("OK\n", out);
        else
            fputs("NOT_FOUND\n", out);
        return true;
    }

    case CMD_TTL: {
        if (!check_argc(cmd, 1, out)) return true;
        int64_t remaining = kvstore_ttl(kv, cmd->argv[1]);
        fprintf(out, "%lld\n", (long long)remaining);
        return true;
    }

    case CMD_COMPACT:
        if (!check_argc(cmd, 0, out)) return true;
        if (kvstore_compact(kv))
            fputs("OK\n", out);
        else
            fputs("ERROR: compaction failed\n", out);
        return true;

    case CMD_EXIT:
    case CMD_QUIT:
        return false;

    case CMD_UNKNOWN:
    default:
        fprintf(out, "ERROR: unknown command '%s'\n",
                cmd->argc ? cmd->argv[0] : "");
        return true;
    }
}
