#include "server.h"
#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>

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

static void *server_thread_run(void *arg)
{
    Server *s = (Server *)arg;
    server_run(s);
    return NULL;
}

int main(void)
{
    printf("Running TCP Server-Client integration tests...\n");

    KVStore *kvkv = kvstore_create();
    TEST_ASSERT(kvkv != NULL, "create KVStore for server");

    ServerConfig cfg = {
        .port = 0, /* ephemeral port */
        .kv   = kvkv
    };

    Server *srv = server_create(&cfg);
    TEST_ASSERT(srv != NULL, "create server");

    TEST_ASSERT(server_start(srv), "start server on ephemeral port");

    int port = server_get_port(srv);
    TEST_ASSERT(port > 0, "valid ephemeral port bound");
    printf("Info: Server bound to port %d\n", port);

    pthread_t th;
    int rc = pthread_create(&th, NULL, server_thread_run, srv);
    TEST_ASSERT(rc == 0, "spawn server run thread");

    /* Brief pause for server to enter poll loop */
    usleep(20000);

    /* Client 1 */
    KVClient client;
    TEST_ASSERT(client_connect(&client, "127.0.0.1", port), "client connect to server");

    char resp[256];

    /* SET with quoted spaces */
    TEST_ASSERT(client_execute(&client, "SET greeting \"hello world\"", resp, sizeof(resp)), "execute SET");
    TEST_ASSERT(strcmp(resp, "OK") == 0, "SET returns OK");

    /* GET */
    TEST_ASSERT(client_execute(&client, "GET greeting", resp, sizeof(resp)), "execute GET");
    TEST_ASSERT(strcmp(resp, "\"hello world\"") == 0, "GET returns quoted value");

    /* COUNT */
    TEST_ASSERT(client_execute(&client, "COUNT", resp, sizeof(resp)), "execute COUNT");
    TEST_ASSERT(strcmp(resp, "1") == 0, "COUNT returns 1");

    /* DEL */
    TEST_ASSERT(client_execute(&client, "DEL greeting", resp, sizeof(resp)), "execute DEL");
    TEST_ASSERT(strcmp(resp, "OK") == 0, "DEL returns OK");

    /* GET after delete */
    TEST_ASSERT(client_execute(&client, "GET greeting", resp, sizeof(resp)), "execute GET after DEL");
    TEST_ASSERT(strcmp(resp, "(nil)") == 0, "GET returns (nil)");

    /* TCP Pipelining Test */
    const char *pipeline_req = "SET k1 1\nSET k2 2\nSET k3 3\n";
    send(client.sock_fd, pipeline_req, strlen(pipeline_req), 0);

    /* Read 3 responses back */
    char r1[32], r2[32], r3[32];
    FILE *fsock = fdopen(dup(client.sock_fd), "r");
    TEST_ASSERT(fsock != NULL, "fdopen dup on socket");
    TEST_ASSERT(fgets(r1, sizeof(r1), fsock) != NULL, "read pipeline response 1");
    TEST_ASSERT(fgets(r2, sizeof(r2), fsock) != NULL, "read pipeline response 2");
    TEST_ASSERT(fgets(r3, sizeof(r3), fsock) != NULL, "read pipeline response 3");
    fclose(fsock);

    /* Trim newlines */
    r1[strcspn(r1, "\r\n")] = 0;
    r2[strcspn(r2, "\r\n")] = 0;
    r3[strcspn(r3, "\r\n")] = 0;

    TEST_ASSERT(strcmp(r1, "OK") == 0, "pipeline response 1 is OK");
    TEST_ASSERT(strcmp(r2, "OK") == 0, "pipeline response 2 is OK");
    TEST_ASSERT(strcmp(r3, "OK") == 0, "pipeline response 3 is OK");

    client_disconnect(&client);

    /* Stop server */
    server_stop(srv);
    pthread_join(th, NULL);

    server_destroy(srv);
    kvstore_destroy(kvkv);

    if (failed_asserts == 0) {
        printf("\nALL TESTS PASSED (%d assertions, 0 failures)\n", total_asserts);
        return 0;
    } else {
        printf("\nTESTS FAILED (%d failures out of %d assertions)\n", failed_asserts, total_asserts);
        return 1;
    }
}
