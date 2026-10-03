#include <stdio.h>
#include <string.h>

#include "dispatcher.h"
#include "kvstore.h"
#include "parser.h"

static void trim_newline(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = '\0';
}

int main(void)
{
    KVStore *kv = kvstore_create();
    if (!kv) {
        fputs("kvstore: out of memory\n", stderr);
        return 1;
    }

    char          line[1024];
    ParsedCommand cmd;

    puts("KVStore REPL — commands: SET k v | GET k | DEL k | COUNT | EXIT");

    while (1) {
        fputs("> ", stdout);
        fflush(stdout);

        if (!fgets(line, sizeof line, stdin))
            break;

        trim_newline(line);
        if (line[0] == '\0')
            continue;

        if (!parse_line(line, &cmd)) {
            fputs("ERROR: parse error\n", stdout);
            continue;
        }

        if (!dispatch_command(kv, &cmd, stdout))
            break;
    }

    kvstore_destroy(kv);
    return 0;
}
