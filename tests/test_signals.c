#include "server.h"
#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

static int total_asserts = 0;
static int failed_asserts = 0;

#define TEST_ASSERT(cond, msg) do { \
    total_asserts++; \
    if (!(cond)) { \
        printf("FAIL: %s (line %d)\n", msg, __LINE__); \
        failed_asserts++; \
    } else { \
        printf("PASS: %s\n", msg); \
    } \
    fflush(stdout); \
} while(0)

static void *server_thread_func(void *arg)
{
    Server *s = (Server *)arg;
    server_run(s);
    return NULL;
}

static void test_programmatic_shutdown(void)
{
    printf("\n--- Test 1: Programmatic stop via self-pipe ---\n");
    KVStore *kv = kvstore_create();
    ServerConfig cfg = { .port = 0, .kv = kv };
    Server *srv = server_create(&cfg);
    TEST_ASSERT(server_start(srv), "server start on ephemeral port");

    pthread_t th;
    pthread_create(&th, NULL, server_thread_func, srv);
    usleep(20000);

    TEST_ASSERT(server_is_running(srv), "server is running");

    /* Stop server programmatically */
    server_stop(srv);
    pthread_join(th, NULL);

    TEST_ASSERT(!server_is_running(srv), "server stopped cleanly");

    server_destroy(srv);
    kvstore_destroy(kv);
}

static void test_sigterm_shutdown(void)
{
    printf("\n--- Test 2: Signal-safe SIGTERM shutdown & drain ---\n");
    KVStore *kv = kvstore_create();
    ServerConfig cfg = { .port = 0, .kv = kv };
    Server *srv = server_create(&cfg);
    TEST_ASSERT(server_start(srv), "server start on ephemeral port");

    server_install_signal_handlers(srv);

    int port = server_get_port(srv);
    pthread_t th;
    pthread_create(&th, NULL, server_thread_func, srv);
    usleep(20000);

    /* Connect a client and perform an operation */
    KVClient client;
    TEST_ASSERT(client_connect(&client, "127.0.0.1", port), "client connected before SIGTERM");
    char resp[128];
    TEST_ASSERT(client_execute(&client, "SET sigkey sigval", resp, sizeof(resp)), "client execute SET");
    TEST_ASSERT(strcmp(resp, "OK") == 0, "SET response OK");

    /* Send SIGTERM to current process */
    printf("Delivering SIGTERM via raise()...\n");
    raise(SIGTERM);

    /* Wait for server thread to exit gracefully */
    pthread_join(th, NULL);
    TEST_ASSERT(!server_is_running(srv), "server exited loop on SIGTERM");

    client_disconnect(&client);
    server_destroy(srv);
    kvstore_destroy(kv);
}

static void test_sigint_shutdown(void)
{
    printf("\n--- Test 3: Signal-safe SIGINT shutdown ---\n");
    KVStore *kv = kvstore_create();
    ServerConfig cfg = { .port = 0, .kv = kv };
    Server *srv = server_create(&cfg);
    TEST_ASSERT(server_start(srv), "server start on ephemeral port");

    server_install_signal_handlers(srv);

    pthread_t th;
    pthread_create(&th, NULL, server_thread_func, srv);
    usleep(20000);

    printf("Delivering SIGINT via raise()...\n");
    raise(SIGINT);

    pthread_join(th, NULL);
    TEST_ASSERT(!server_is_running(srv), "server exited loop on SIGINT");

    server_destroy(srv);
    kvstore_destroy(kv);
}

int main(void)
{
    printf("Running Signal Handling & Graceful Shutdown tests (Week 6)...\n");
    test_programmatic_shutdown();
    test_sigterm_shutdown();
    test_sigint_shutdown();

    if (failed_asserts == 0) {
        printf("\nALL TESTS PASSED (%d assertions, 0 failures)\n", total_asserts);
        return 0;
    } else {
        printf("\nTESTS FAILED (%d failures out of %d assertions)\n", failed_asserts, total_asserts);
        return 1;
    }
}
