#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int  sock_fd;
    char host[256];
    int  port;
} KVClient;

/* Connects to the KVStore server at host:port. Returns true on success. */
bool client_connect(KVClient *client, const char *host, int port);

/* Disconnects and closes the socket. */
void client_disconnect(KVClient *client);

/* Sends a command string and reads one line response into out_resp. */
bool client_execute(KVClient *client, const char *cmd, char *out_resp, size_t max_resp);

#endif /* CLIENT_H */
