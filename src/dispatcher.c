#include "dispatcher.h"

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
        if (!check_argc(cmd, 2, out)) return true;
        if (kvstore_set(kv, cmd->argv[1], cmd->argv[2]))
            fputs("OK\n", out);
        else
            fputs("ERROR: out of memory\n", out);
        return true;

    case CMD_GET: {
        if (!check_argc(cmd, 1, out)) return true;
        const char *v = kvstore_get(kv, cmd->argv[1]);
        if (v) fprintf(out, "\"%s\"\n", v);
        else   fputs("(nil)\n", out);
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
