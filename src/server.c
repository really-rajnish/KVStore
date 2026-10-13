#include "server.h"
#include "protocol.h"
#include "parser.h"
#include "dispatcher.h"
#include "threadpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_WORKER_THREADS 8
#define SERVER_QUEUE_CAPACITY 256

static Server *g_signal_server = NULL;

typedef struct ClientNode {
    int                client_fd;
    struct ClientNode *next;
} ClientNode;

struct Server {
    int              port;
    int              listen_fd;
    int              self_pipe[2];
    volatile bool    running;
    volatile bool    draining;
    KVStore         *kv;
    ThreadPool      *pool;

    pthread_mutex_t  clients_lock;
    pthread_cond_t   clients_cond;
    ClientNode      *clients_head;
    size_t           active_clients;
};

static void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

static void sig_handler(int sig)
{
    (void)sig;
    if (g_signal_server && g_signal_server->self_pipe[1] >= 0) {
        char b = 'x';
        ssize_t ret = write(g_signal_server->self_pipe[1], &b, 1);
        (void)ret;
    }
}

typedef struct {
    Server *server;
    int     client_fd;
} ClientTaskArgs;

static void server_add_client(Server *server, int client_fd)
{
    pthread_mutex_lock(&server->clients_lock);
    ClientNode *node = malloc(sizeof *node);
    if (node) {
        node->client_fd = client_fd;
        node->next      = server->clients_head;
        server->clients_head = node;
        server->active_clients++;
    }
    pthread_mutex_unlock(&server->clients_lock);
}

static void server_remove_client(Server *server, int client_fd)
{
    pthread_mutex_lock(&server->clients_lock);
    ClientNode **curr = &server->clients_head;
    while (*curr) {
        if ((*curr)->client_fd == client_fd) {
            ClientNode *to_free = *curr;
            *curr = (*curr)->next;
            free(to_free);
            if (server->active_clients > 0)
                server->active_clients--;
            break;
        }
        curr = &(*curr)->next;
    }
    if (server->active_clients == 0) {
        pthread_cond_broadcast(&server->clients_cond);
    }
    pthread_mutex_unlock(&server->clients_lock);
}

static void client_task_fn(void *arg)
{
    ClientTaskArgs *args = (ClientTaskArgs *)arg;
    Server *server = args->server;
    int client_fd = args->client_fd;
    free(args);

    server_handle_client(server, client_fd);
    server_remove_client(server, client_fd);
}

Server *server_create(const ServerConfig *config)
{
    if (!config || !config->kv) return NULL;

    Server *server = malloc(sizeof *server);
    if (!server) return NULL;

    server->port           = config->port;
    server->listen_fd      = -1;
    server->self_pipe[0]   = -1;
    server->self_pipe[1]   = -1;
    server->running        = false;
    server->draining       = false;
    server->kv             = config->kv;
    server->clients_head   = NULL;
    server->active_clients = 0;

    server->pool = threadpool_create(SERVER_WORKER_THREADS, SERVER_QUEUE_CAPACITY);
    if (!server->pool) {
        free(server);
        return NULL;
    }

    pthread_mutex_init(&server->clients_lock, NULL);
    pthread_cond_init(&server->clients_cond, NULL);

    if (pipe(server->self_pipe) == 0) {
        set_nonblocking(server->self_pipe[0]);
        set_nonblocking(server->self_pipe[1]);
    }

    return server;
}

void server_install_signal_handlers(Server *server)
{
    g_signal_server = server;

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    signal(SIGPIPE, SIG_IGN);
}

void server_destroy(Server *server)
{
    if (!server) return;
    server_stop(server);

    if (g_signal_server == server) {
        g_signal_server = NULL;
    }

    if (server->pool) {
        threadpool_destroy(server->pool);
        server->pool = NULL;
    }

    if (server->self_pipe[0] >= 0) close(server->self_pipe[0]);
    if (server->self_pipe[1] >= 0) close(server->self_pipe[1]);

    pthread_mutex_destroy(&server->clients_lock);
    pthread_cond_destroy(&server->clients_cond);
    free(server);
}

bool server_start(Server *server)
{
    if (!server) return false;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return false;

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons((uint16_t)server->port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return false;
    }

    if (listen(fd, SERVER_BACKLOG) < 0) {
        close(fd);
        return false;
    }

    socklen_t addr_len = sizeof(addr);
    if (getsockname(fd, (struct sockaddr *)&addr, &addr_len) == 0) {
        server->port = ntohs(addr.sin_port);
    }

    server->listen_fd = fd;
    server->running   = true;
    server->draining  = false;
    return true;
}

void server_run(Server *server)
{
    if (!server || server->listen_fd < 0) return;

    struct pollfd pfds[2];
    pfds[0].fd     = server->listen_fd;
    pfds[0].events = POLLIN;

    pfds[1].fd     = server->self_pipe[0];
    pfds[1].events = POLLIN;

    while (server->running) {
        pfds[0].revents = 0;
        pfds[1].revents = 0;

        int ret = poll(pfds, 2, -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (pfds[1].revents & POLLIN) {
            char dump[32];
            while (read(server->self_pipe[0], dump, sizeof(dump)) > 0) {}
            server->running = false;
            break;
        }

        if (pfds[0].revents & POLLIN) {
            int client_fd = accept(server->listen_fd, NULL, NULL);
            if (client_fd < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;
                break;
            }

            if (!server->running || server->draining) {
                protocol_send_response(client_fd, "ERROR: server shutting down\n");
                close(client_fd);
                continue;
            }

            ClientTaskArgs *args = malloc(sizeof *args);
            if (!args) {
                close(client_fd);
                continue;
            }
            args->server    = server;
            args->client_fd = client_fd;

            server_add_client(server, client_fd);
            if (!threadpool_submit(server->pool, client_task_fn, args)) {
                server_remove_client(server, client_fd);
                close(client_fd);
                free(args);
            }
        }
    }

    /* Drain phase */
    server->draining = true;
    if (server->listen_fd >= 0) {
        close(server->listen_fd);
        server->listen_fd = -1;
    }

    pthread_mutex_lock(&server->clients_lock);
    for (ClientNode *c = server->clients_head; c != NULL; c = c->next) {
        shutdown(c->client_fd, SHUT_RDWR);
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 2;

    while (server->active_clients > 0) {
        int wait_rc = pthread_cond_timedwait(&server->clients_cond, &server->clients_lock, &ts);
        if (wait_rc == ETIMEDOUT) {
            break;
        }
    }
    pthread_mutex_unlock(&server->clients_lock);
}

void server_stop(Server *server)
{
    if (!server) return;
    server->running = false;

    if (server->self_pipe[1] >= 0) {
        char b = 'x';
        ssize_t ret = write(server->self_pipe[1], &b, 1);
        (void)ret;
    }
}

bool server_is_running(const Server *server)
{
    return server ? server->running : false;
}

int server_get_port(const Server *server)
{
    return server ? server->port : -1;
}

void server_handle_client(Server *server, int client_fd)
{
    if (!server || client_fd < 0) return;

    ClientBuffer cb;
    protocol_buffer_init(&cb);

    char chunk[1024];
    char line[PROTOCOL_MAX_LINE];
    bool should_close = false;

    while (!should_close) {
        ssize_t n = recv(client_fd, chunk, sizeof(chunk), 0);
        if (n <= 0) {
            break;
        }

        if (!protocol_buffer_append(&cb, chunk, (size_t)n)) {
            break;
        }

        while (protocol_extract_line(&cb, line, sizeof(line))) {
            if (line[0] == '\0') continue;

            ParsedCommand cmd;
            if (!parse_line(line, &cmd)) {
                protocol_send_response(client_fd, "ERROR: parse error\n");
                continue;
            }

            char *resp_buf = NULL;
            size_t resp_len = 0;
            FILE *out = open_memstream(&resp_buf, &resp_len);
            if (!out) {
                protocol_send_response(client_fd, "ERROR: out of memory\n");
                continue;
            }

            bool cont = dispatch_command(server->kv, &cmd, out);
            fclose(out);

            if (resp_buf && resp_len > 0) {
                protocol_send_all(client_fd, resp_buf, resp_len);
            }
            free(resp_buf);

            if (!cont) {
                should_close = true;
                break;
            }
        }
    }

    protocol_buffer_free(&cb);
    close(client_fd);
}
