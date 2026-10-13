#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "client.h"

static void trim_newline(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = '\0';
}

int main(int argc, char **argv)
{
    const char *host = "127.0.0.1";
    int port = 6379;
    int opt;

    while ((opt = getopt(argc, argv, "h:p:")) != -1) {
        switch (opt) {
        case 'h':
            host = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-h host] [-p port] [command...]\n", argv[0]);
            return 1;
        }
    }

    KVClient client;
    if (!client_connect(&client, host, port)) {
        fprintf(stderr, "Failed to connect to %s:%d\n", host, port);
        return 1;
    }

    if (optind < argc) {
        char cmd[1024] = "";
        for (int i = optind; i < argc; i++) {
            if (i > optind) strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1);
            strncat(cmd, argv[i], sizeof(cmd) - strlen(cmd) - 1);
        }
        char resp[4096];
        if (client_execute(&client, cmd, resp, sizeof(resp))) {
            printf("%s\n", resp);
            client_disconnect(&client);
            return 0;
        } else {
            fprintf(stderr, "Failed to execute command\n");
            client_disconnect(&client);
            return 1;
        }
    }

    printf("Connected to KVStore at %s:%d\n", host, port);
    printf("Type commands (SET, GET, DEL, COUNT, EXIT)\n");

    char line[1024];
    char resp[4096];
    while (1) {
        printf("kvstore> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin))
            break;
        trim_newline(line);
        if (line[0] == '\0')
            continue;
        if (strcasecmp(line, "EXIT") == 0 || strcasecmp(line, "QUIT") == 0) {
            client_execute(&client, line, resp, sizeof(resp));
            break;
        }
        if (client_execute(&client, line, resp, sizeof(resp))) {
            printf("%s\n", resp);
        } else {
            printf("Connection closed by server\n");
            break;
        }
    }

    client_disconnect(&client);
    return 0;
}
