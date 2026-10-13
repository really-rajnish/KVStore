#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "server.h"
#include "kvstore.h"

int main(int argc, char **argv)
{
    int port = DEFAULT_SERVER_PORT;
    const char *wal_path = "data/kvstore.wal";
    int opt;
    while ((opt = getopt(argc, argv, "p:d:h")) != -1) {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            if (port <= 0 || port > 65535) {
                fprintf(stderr, "Invalid port: %s\n", optarg);
                return 1;
            }
            break;
        case 'd':
            wal_path = optarg;
            break;
        case 'h':
        default:
            fprintf(stderr, "Usage: %s [-p port] [-d wal_path]\n", argv[0]);
            return 1;
        }
    }

    KVStore *kv = kvstore_create_with_wal(wal_path, WAL_SYNC_ALWAYS);
    if (!kv) {
        fprintf(stderr, "Failed to create KVStore with WAL: %s\n", wal_path);
        return 1;
    }

    /* Start active background TTL reaper */
    kvstore_start_ttl_reaper(kv, 100);

    ServerConfig config = {
        .port = port,
        .kv   = kv
    };

    Server *server = server_create(&config);
    if (!server) {
        fprintf(stderr, "Failed to create server\n");
        kvstore_destroy(kv);
        return 1;
    }

    server_install_signal_handlers(server);

    if (!server_start(server)) {
        fprintf(stderr, "Failed to bind and listen on port %d\n", port);
        server_destroy(server);
        kvstore_destroy(kv);
        return 1;
    }

    printf("KVStore server listening on port %d (WAL: %s)\n", server_get_port(server), wal_path);
    server_run(server);

    printf("Graceful shutdown complete.\n");
    server_destroy(server);
    kvstore_destroy(kv);
    return 0;
}
