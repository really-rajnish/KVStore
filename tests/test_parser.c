#include <stdio.h>
#include <string.h>

#include "parser.h"

static int failures = 0;

#define CHECK(cond, msg)                                         \
    do {                                                         \
        if (cond) { printf("PASS  %s\n", (msg)); }               \
        else      { printf("FAIL  %s\n", (msg)); failures++; }   \
    } while (0)

static void test_basic(void)
{
    ParsedCommand c;
    CHECK(parse_line("SET name Alice", &c), "parse SET");
    CHECK(c.type == CMD_SET,                "type = SET");
    CHECK(c.argc == 3,                      "argc = 3");
    CHECK(strcmp(c.argv[0], "SET")   == 0,  "argv[0]");
    CHECK(strcmp(c.argv[1], "name")  == 0,  "argv[1]");
    CHECK(strcmp(c.argv[2], "Alice") == 0,  "argv[2]");
}

static void test_quoted(void)
{
    ParsedCommand c;
    CHECK(parse_line("SET greeting \"hello world\"", &c), "parse quoted SET");
    CHECK(c.argc == 3,                                    "argc = 3");
    CHECK(strcmp(c.argv[2], "hello world") == 0,          "quoted value kept spaces");
}

static void test_escapes(void)
{
    ParsedCommand c;
    CHECK(parse_line("SET path \"a\\\"b\"", &c),          "escaped quote parses");
    CHECK(strcmp(c.argv[2], "a\"b") == 0,                 "escape decoded");
}

static void test_get_del(void)
{
    ParsedCommand c;
    CHECK(parse_line("GET name", &c),                     "parse GET");
    CHECK(c.type == CMD_GET,                              "type = GET");
    CHECK(c.argc == 2,                                    "argc = 2");

    CHECK(parse_line("DEL name", &c),                     "parse DEL");
    CHECK(c.type == CMD_DEL,                              "type = DEL");
    CHECK(c.argc == 2,                                    "argc = 2");
}

static void test_count_exit(void)
{
    ParsedCommand c;
    CHECK(parse_line("COUNT", &c),                        "parse COUNT");
    CHECK(c.type == CMD_COUNT,                            "type = COUNT");
    CHECK(c.argc == 1,                                    "argc = 1");

    CHECK(parse_line("EXIT", &c),                         "parse EXIT");
    CHECK(c.type == CMD_EXIT,                             "type = EXIT");
}

static void test_unknown(void)
{
    ParsedCommand c;
    CHECK(parse_line("FOO bar", &c),                      "parse FOO");
    CHECK(c.type == CMD_UNKNOWN,                          "type = UNKNOWN");
    CHECK(c.argc == 2,                                    "argc = 2");
}

static void test_whitespace(void)
{
    ParsedCommand c;
    CHECK(parse_line("   SET\tkey   value   ", &c),       "tabs and spaces");
    CHECK(c.type == CMD_SET,                              "type = SET");
    CHECK(c.argc == 3,                                    "argc = 3");
    CHECK(strcmp(c.argv[1], "key")   == 0,                "key");
    CHECK(strcmp(c.argv[2], "value") == 0,                "value");
}

static void test_empty(void)
{
    ParsedCommand c;
    CHECK(!parse_line("", &c),                            "empty fails");
    CHECK(!parse_line("   \t   ", &c),                    "whitespace-only fails");
}

static void test_unterminated_quote(void)
{
    ParsedCommand c;
    CHECK(!parse_line("SET k \"unterminated", &c),        "unterminated quote fails");
}

int main(void)
{
    test_basic();
    test_quoted();
    test_escapes();
    test_get_del();
    test_count_exit();
    test_unknown();
    test_whitespace();
    test_empty();
    test_unterminated_quote();

    printf("\n%s (%d failure%s)\n",
           failures ? "TESTS FAILED" : "ALL TESTS PASSED",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
