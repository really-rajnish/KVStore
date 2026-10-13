#include "client.h"
#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

bool client_connect(KVClient *client, const char *host, int port)
{
    if (!client || !host || port <= 0) return false;
    client->sock_fd = -1;
    strncpy(client->host, host, sizeof(client->host) - 1);
    client->host[sizeof(client->host) - 1] = '\0';
    client->port = port;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        return false;
    }

    int fd = -1;
    for (p = res; p != NULL; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd < 0) return false;
    client->sock_fd = fd;
    return true;
}

void client_disconnect(KVClient *client)
{
    if (!client) return;
    if (client->sock_fd >= 0) {
        close(client->sock_fd);
        client->sock_fd = -1;
    }
}

bool client_execute(KVClient *client, const char *cmd, char *out_resp, size_t max_resp)
{
    if (!client || client->sock_fd < 0 || !cmd || !out_resp || max_resp == 0)
        return false;

    size_t cmd_len = strlen(cmd);
    bool has_nl = (cmd_len > 0 && cmd[cmd_len - 1] == '\n');

    if (!protocol_send_all(client->sock_fd, cmd, cmd_len))
        return false;
    if (!has_nl) {
        if (!protocol_send_all(client->sock_fd, "\n", 1))
            return false;
    }

    ClientBuffer cb;
    protocol_buffer_init(&cb);
    char chunk[256];
    bool got_line = false;

    while (!got_line) {
        ssize_t n = recv(client->sock_fd, chunk, sizeof(chunk), 0);
        if (n <= 0) break;
        if (!protocol_buffer_append(&cb, chunk, (size_t)n)) break;
        if (protocol_extract_line(&cb, out_resp, max_resp)) {
            got_line = true;
            break;
        }
    }

    protocol_buffer_free(&cb);
    return got_line;
}
