#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
} while(0)

static void test_basic(void)
{
    ClientBuffer cb;
    protocol_buffer_init(&cb);

    const char *msg = "SET key value\n";
    TEST_ASSERT(protocol_buffer_append(&cb, msg, strlen(msg)), "append basic line");

    char line[128];
    TEST_ASSERT(protocol_extract_line(&cb, line, sizeof(line)), "extract basic line");
    TEST_ASSERT(strcmp(line, "SET key value") == 0, "line content matches");
    TEST_ASSERT(cb.length == 0, "buffer is now empty");

    protocol_buffer_free(&cb);
}

static void test_partial_reads(void)
{
    ClientBuffer cb;
    protocol_buffer_init(&cb);

    const char *cmd = "GET user:100\n";
    size_t len = strlen(cmd);
    char line[128];

    for (size_t i = 0; i < len - 1; i++) {
        TEST_ASSERT(protocol_buffer_append(&cb, &cmd[i], 1), "append 1 byte");
        TEST_ASSERT(!protocol_extract_line(&cb, line, sizeof(line)), "no line before newline");
    }

    TEST_ASSERT(protocol_buffer_append(&cb, &cmd[len - 1], 1), "append newline byte");
    TEST_ASSERT(protocol_extract_line(&cb, line, sizeof(line)), "line extracted after newline");
    TEST_ASSERT(strcmp(line, "GET user:100") == 0, "extracted line matches");
    TEST_ASSERT(cb.length == 0, "buffer empty");

    protocol_buffer_free(&cb);
}

static void test_pipelining(void)
{
    ClientBuffer cb;
    protocol_buffer_init(&cb);

    const char *pipeline = "SET a 1\nGET a\nDEL a\n";
    TEST_ASSERT(protocol_buffer_append(&cb, pipeline, strlen(pipeline)), "append pipeline buffer");

    char line[128];
    TEST_ASSERT(protocol_extract_line(&cb, line, sizeof(line)), "extract pipeline line 1");
    TEST_ASSERT(strcmp(line, "SET a 1") == 0, "pipeline line 1 content");

    TEST_ASSERT(protocol_extract_line(&cb, line, sizeof(line)), "extract pipeline line 2");
    TEST_ASSERT(strcmp(line, "GET a") == 0, "pipeline line 2 content");

    TEST_ASSERT(protocol_extract_line(&cb, line, sizeof(line)), "extract pipeline line 3");
    TEST_ASSERT(strcmp(line, "DEL a") == 0, "pipeline line 3 content");

    TEST_ASSERT(!protocol_extract_line(&cb, line, sizeof(line)), "no line 4 in pipeline");
    TEST_ASSERT(cb.length == 0, "buffer empty after pipeline");

    protocol_buffer_free(&cb);
}

static void test_trailing_partial(void)
{
    ClientBuffer cb;
    protocol_buffer_init(&cb);

    const char *part1 = "SET score 42\nGE";
    TEST_ASSERT(protocol_buffer_append(&cb, part1, strlen(part1)), "append complete + partial");

    char line[128];
    TEST_ASSERT(protocol_extract_line(&cb, line, sizeof(line)), "extract complete line");
    TEST_ASSERT(strcmp(line, "SET score 42") == 0, "complete line content");

    TEST_ASSERT(!protocol_extract_line(&cb, line, sizeof(line)), "cannot extract incomplete trailing command");
    TEST_ASSERT(cb.length == 2, "partial bytes remaining in buffer");

    const char *part2 = "T score\n";
    TEST_ASSERT(protocol_buffer_append(&cb, part2, strlen(part2)), "append completion");
    TEST_ASSERT(protocol_extract_line(&cb, line, sizeof(line)), "extract completed trailing command");
    TEST_ASSERT(strcmp(line, "GET score") == 0, "completed trailing content");
    TEST_ASSERT(cb.length == 0, "buffer empty");

    protocol_buffer_free(&cb);
}

static void test_crlf_and_empty(void)
{
    ClientBuffer cb;
    protocol_buffer_init(&cb);

    const char *crlf = "COUNT\r\n\r\n";
    TEST_ASSERT(protocol_buffer_append(&cb, crlf, strlen(crlf)), "append CRLF commands");

    char line[128];
    TEST_ASSERT(protocol_extract_line(&cb, line, sizeof(line)), "extract CRLF line 1");
    TEST_ASSERT(strcmp(line, "COUNT") == 0, "CRLF stripped correctly");

    TEST_ASSERT(protocol_extract_line(&cb, line, sizeof(line)), "extract CRLF empty line");
    TEST_ASSERT(strcmp(line, "") == 0, "empty line extracted");

    TEST_ASSERT(!protocol_extract_line(&cb, line, sizeof(line)), "no more lines");
    protocol_buffer_free(&cb);
}

int main(void)
{
    printf("Running protocol unit tests...\n");
    test_basic();
    test_partial_reads();
    test_pipelining();
    test_trailing_partial();
    test_crlf_and_empty();

    if (failed_asserts == 0) {
        printf("\nALL TESTS PASSED (%d assertions, 0 failures)\n", total_asserts);
        return 0;
    } else {
        printf("\nTESTS FAILED (%d failures out of %d assertions)\n", failed_asserts, total_asserts);
        return 1;
    }
}
