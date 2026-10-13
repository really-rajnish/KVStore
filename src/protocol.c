#include "protocol.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

void protocol_buffer_init(ClientBuffer *cb)
{
    if (!cb) return;
    cb->buf = malloc(PROTOCOL_INITIAL_CAP);
    cb->capacity = cb->buf ? PROTOCOL_INITIAL_CAP : 0;
    cb->length = 0;
}

void protocol_buffer_free(ClientBuffer *cb)
{
    if (!cb) return;
    free(cb->buf);
    cb->buf = NULL;
    cb->capacity = 0;
    cb->length = 0;
}

bool protocol_buffer_append(ClientBuffer *cb, const char *data, size_t len)
{
    if (!cb || !data || len == 0) return true;

    if (cb->length + len > cb->capacity) {
        size_t new_cap = cb->capacity ? cb->capacity : PROTOCOL_INITIAL_CAP;
        while (new_cap < cb->length + len) {
            new_cap *= 2;
        }
        char *nb = realloc(cb->buf, new_cap);
        if (!nb) return false;
        cb->buf = nb;
        cb->capacity = new_cap;
    }

    memcpy(cb->buf + cb->length, data, len);
    cb->length += len;
    return true;
}

bool protocol_extract_line(ClientBuffer *cb, char *out_line, size_t max_len)
{
    if (!cb || !cb->buf || cb->length == 0 || !out_line || max_len == 0)
        return false;

    char *nl = memchr(cb->buf, '\n', cb->length);
    if (!nl) return false;

    size_t line_len = (size_t)(nl - cb->buf);
    size_t copy_len = line_len;

    /* Strip optional trailing \r */
    if (copy_len > 0 && cb->buf[copy_len - 1] == '\r') {
        copy_len--;
    }

    if (copy_len >= max_len) {
        copy_len = max_len - 1;
    }

    memcpy(out_line, cb->buf, copy_len);
    out_line[copy_len] = '\0';

    /* Advance buffer past the \n */
    size_t total_consumed = line_len + 1;
    size_t remaining = cb->length - total_consumed;
    if (remaining > 0) {
        memmove(cb->buf, cb->buf + total_consumed, remaining);
    }
    cb->length = remaining;

    return true;
}

bool protocol_send_all(int fd, const char *data, size_t len)
{
    if (fd < 0 || !data) return false;

    size_t total_sent = 0;
    while (total_sent < len) {
        ssize_t sent = send(fd, data + total_sent, len - total_sent, MSG_NOSIGNAL);
        if (sent < 0) {
            return false;
        }
        total_sent += (size_t)sent;
    }
    return true;
}

bool protocol_send_response(int fd, const char *resp)
{
    if (!resp) return false;
    return protocol_send_all(fd, resp, strlen(resp));
}
