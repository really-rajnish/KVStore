#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stddef.h>
#include "kvstore.h"

#define DEFAULT_SERVER_PORT 6379
#define SERVER_BACKLOG      128

typedef struct Server Server;

typedef struct {
    int      port;
    KVStore *kv;
} ServerConfig;

Server *server_create(const ServerConfig *config);
void    server_destroy(Server *server);

/* Installs async-signal-safe sigaction handlers for SIGINT and SIGTERM via self-pipe trick. */
void    server_install_signal_handlers(Server *server);

/* Creates and binds listening socket. */
bool    server_start(Server *server);

/* Runs the accept loop until signal or server_stop() is received. */
void    server_run(Server *server);

/* Signals the server to stop accepting connections and drain active clients. */
void    server_stop(Server *server);

/* Returns whether the server is currently running. */
bool    server_is_running(const Server *server);

/* Returns the actual port bound (helpful for ephemeral port 0). */
int     server_get_port(const Server *server);

/* Handles a single client connection until closure. */
void    server_handle_client(Server *server, int client_fd);

#endif /* SERVER_H */
