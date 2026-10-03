#include "parser.h"

#include <string.h>

/* ---------------- token reader ---------------- */

typedef enum { TOK_OK, TOK_END, TOK_ERR } TokStatus;

static TokStatus read_token(const char **p, char *out, size_t cap)
{
    const char *s = *p;

    while (*s == ' ' || *s == '\t') s++;
    if (*s == '\0') { *p = s; return TOK_END; }

    size_t i = 0;

    if (*s == '"') {
        s++;
        while (*s && *s != '"') {
            char c = *s;
            if (c == '\\' && (s[1] == '"' || s[1] == '\\')) {
                s++;
                c = *s;
            }
            if (i + 1 >= cap) { *p = s; return TOK_ERR; }
            out[i++] = c;
            s++;
        }
        if (*s != '"') { *p = s; return TOK_ERR; }
        s++;
    } else {
        while (*s && *s != ' ' && *s != '\t') {
            if (i + 1 >= cap) { *p = s; return TOK_ERR; }
            out[i++] = *s++;
        }
    }

    out[i] = '\0';
    *p = s;
    return TOK_OK;
}

/* ---------------- command table ---------------- */

CommandType command_type_from_name(const char *name)
{
    if (!name) return CMD_UNKNOWN;
    if (strcmp(name, "SET")   == 0) return CMD_SET;
    if (strcmp(name, "GET")   == 0) return CMD_GET;
    if (strcmp(name, "DEL")   == 0) return CMD_DEL;
    if (strcmp(name, "COUNT") == 0) return CMD_COUNT;
    if (strcmp(name, "EXIT")  == 0) return CMD_EXIT;
    if (strcmp(name, "QUIT")  == 0) return CMD_QUIT;
    return CMD_UNKNOWN;
}

const char *command_type_name(CommandType t)
{
    switch (t) {
        case CMD_SET:   return "SET";
        case CMD_GET:   return "GET";
        case CMD_DEL:   return "DEL";
        case CMD_COUNT: return "COUNT";
        case CMD_EXIT:  return "EXIT";
        case CMD_QUIT:  return "QUIT";
        default:        return "UNKNOWN";
    }
}

/* ---------------- public API ---------------- */

bool parse_line(const char *line, ParsedCommand *out)
{
    if (!line || !out) return false;
    memset(out, 0, sizeof *out);

    const char *p = line;
    size_t n = 0;

    while (n < PARSER_MAX_ARGS) {
        TokStatus st = read_token(&p, out->storage[n], PARSER_MAX_TOKEN);
        if (st == TOK_END) break;
        if (st == TOK_ERR) return false;
        out->argv[n] = out->storage[n];
        n++;
    }

    if (n == PARSER_MAX_ARGS) {
        const char *q = p;
        while (*q == ' ' || *q == '\t') q++;
        if (*q != '\0') return false;
    }

    out->argc = n;
    if (n == 0) return false;

    out->type = command_type_from_name(out->argv[0]);
    return true;
}
