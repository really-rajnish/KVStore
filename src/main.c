#include <stdio.h>
#include <string.h>

#include "kvstore.h"

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
        fprintf(stderr, "kvstore: out of memory\n");
        return 1;
    }

    char line[1024];
    puts("KVStore REPL — commands: SET k v | GET k | DEL k | COUNT | EXIT");

    while (1) {
        fputs("> ", stdout);
        fflush(stdout);

        if (!fgets(line, sizeof line, stdin))
            break;

        trim_newline(line);
        if (line[0] == '\0')
            continue;

        char cmd[16], key[256], value[512];
        int n = sscanf(line, "%15s %255s %511[^\n]", cmd, key, value);

        if (strcmp(cmd, "SET") == 0) {
            if (n < 3) {
                puts("ERROR: usage: SET key value");
            } else {
                puts(kvstore_set(kv, key, value) ? "OK" : "ERROR");
            }
        } else if (strcmp(cmd, "GET") == 0) {
            if (n < 2) {
                puts("ERROR: usage: GET key");
            } else {
                char *v = kvstore_get(kv, key);
                if (v) printf("\"%s\"\n", v);
                else   puts("(nil)");
            }
        } else if (strcmp(cmd, "DEL") == 0) {
            if (n < 2) {
                puts("ERROR: usage: DEL key");
            } else {
                puts(kvstore_del(kv, key) ? "OK" : "NOT_FOUND");
            }
        } else if (strcmp(cmd, "COUNT") == 0) {
            printf("%zu\n", kvstore_size(kv));
        } else if (strcmp(cmd, "EXIT") == 0 || strcmp(cmd, "QUIT") == 0) {
            break;
        } else {
            puts("ERROR: unknown command");
        }
    }

    kvstore_destroy(kv);
    return 0;
}
