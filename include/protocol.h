#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <stdbool.h>

#define PROTOCOL_INITIAL_CAP 1024
#define PROTOCOL_MAX_LINE    4096

typedef struct {
    char   *buf;
    size_t  capacity;
    size_t  length;
} ClientBuffer;

void protocol_buffer_init(ClientBuffer *cb);
void protocol_buffer_free(ClientBuffer *cb);

/* Appends raw data received from socket. Resizes dynamically if needed. */
bool protocol_buffer_append(ClientBuffer *cb, const char *data, size_t len);

/* Extracts the next newline-terminated line from cb into out_line.
   Strips optional trailing \r. Null-terminates out_line.
   Shifts remaining buffer bytes to the front.
   Returns true if a full line was extracted, false otherwise. */
bool protocol_extract_line(ClientBuffer *cb, char *out_line, size_t max_len);

/* Sends all bytes over socket fd, handling partial send loops. */
bool protocol_send_all(int fd, const char *data, size_t len);

/* Sends a null-terminated response string over socket fd. */
bool protocol_send_response(int fd, const char *resp);

#endif /* PROTOCOL_H */
