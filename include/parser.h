#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdbool.h>

#define PARSER_MAX_ARGS   8
#define PARSER_MAX_TOKEN  256

typedef enum {
    CMD_UNKNOWN = 0,
    CMD_SET,
    CMD_GET,
    CMD_DEL,
    CMD_COUNT,
    CMD_EXIT,
    CMD_QUIT
} CommandType;

typedef struct {
    CommandType  type;
    size_t       argc;
    const char  *argv[PARSER_MAX_ARGS];
    char         storage[PARSER_MAX_ARGS][PARSER_MAX_TOKEN];
} ParsedCommand;

/* Parse a single line into a ParsedCommand.
   Returns false on empty input, unterminated quotes, or too many tokens. */
bool parse_line(const char *line, ParsedCommand *out);

CommandType command_type_from_name(const char *name);
const char *command_type_name(CommandType t);

#endif /* PARSER_H */
