#include "wal.h"
#include "ttl.h"
#include "hash_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/stat.h>

struct Wal {
    int           fd;
    char         *filepath;
    WalSyncPolicy sync_policy;
};

#pragma pack(push, 1)
typedef struct {
    uint8_t  op;
    uint16_t key_len;
    uint32_t val_len;
} WalHeader;
#pragma pack(pop)

static inline uint64_t host_to_be64(uint64_t val) {
    return (((uint64_t)htonl((uint32_t)(val & 0xFFFFFFFF))) << 32) |
           htonl((uint32_t)(val >> 32));
}

static inline uint64_t be64_to_host(uint64_t val) {
    return (((uint64_t)ntohl((uint32_t)(val & 0xFFFFFFFF))) << 32) |
           ntohl((uint32_t)(val >> 32));
}

static uint32_t crc32_byte(uint32_t r) {
    for (int j = 0; j < 8; ++j)
        r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ (r >> 1);
    return r ^ (uint32_t)0xFF000000L;
}

uint32_t wal_crc32(const void *data, size_t len) {
    static uint32_t table[256];
    static int have_table = 0;
    if (!have_table) {
        for (size_t i = 0; i < 256; ++i)
            table[i] = crc32_byte((uint32_t)i);
        have_table = 1;
    }
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; ++i)
        crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

Wal *wal_open(const char *filepath, WalSyncPolicy sync_policy)
{
    if (!filepath) return NULL;

    char *dir_copy = strdup(filepath);
    char *slash = strrchr(dir_copy, '/');
    if (slash) {
        *slash = '\0';
        mkdir(dir_copy, 0755);
    }
    free(dir_copy);

    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return NULL;

    Wal *wal = malloc(sizeof *wal);
    if (!wal) {
        close(fd);
        return NULL;
    }

    wal->fd          = fd;
    wal->filepath    = strdup(filepath);
    wal->sync_policy = sync_policy;
    return wal;
}

void wal_close(Wal *wal)
{
    if (!wal) return;
    if (wal->fd >= 0) {
        wal_sync(wal);
        close(wal->fd);
    }
    free(wal->filepath);
    free(wal);
}

bool wal_sync(Wal *wal)
{
    if (!wal || wal->fd < 0) return false;
    return fsync(wal->fd) == 0;
}

static bool wal_append_record(Wal *wal, uint8_t op, const char *key, const char *val, int64_t expire_at_ms)
{
    if (!wal || wal->fd < 0 || !key) return false;

    size_t key_len = strlen(key);
    size_t val_len = val ? strlen(val) : 0;
    if (key_len > 0xFFFF) return false;

    size_t extra_len = (op == WAL_OP_SET_TTL) ? sizeof(uint64_t) : 0;
    size_t payload_len = sizeof(WalHeader) + key_len + val_len + extra_len;
    size_t total_len   = payload_len + sizeof(uint32_t);

    uint8_t *buf = malloc(total_len);
    if (!buf) return false;

    WalHeader hdr;
    hdr.op      = op;
    hdr.key_len = htons((uint16_t)key_len);
    hdr.val_len = htonl((uint32_t)val_len);

    size_t off = 0;
    memcpy(buf + off, &hdr, sizeof(WalHeader));
    off += sizeof(WalHeader);

    memcpy(buf + off, key, key_len);
    off += key_len;

    if (val_len > 0) {
        memcpy(buf + off, val, val_len);
        off += val_len;
    }

    if (op == WAL_OP_SET_TTL) {
        uint64_t exp_net = host_to_be64((uint64_t)expire_at_ms);
        memcpy(buf + off, &exp_net, sizeof(uint64_t));
        off += sizeof(uint64_t);
    }

    uint32_t crc = wal_crc32(buf, payload_len);
    uint32_t crc_net = htonl(crc);
    memcpy(buf + payload_len, &crc_net, sizeof(uint32_t));

    size_t written = 0;
    while (written < total_len) {
        ssize_t n = write(wal->fd, buf + written, total_len - written);
        if (n <= 0) {
            free(buf);
            return false;
        }
        written += (size_t)n;
    }
    free(buf);

    if (wal->sync_policy == WAL_SYNC_ALWAYS) {
        wal_sync(wal);
    }
    return true;
}

bool wal_append_set(Wal *wal, const char *key, const char *value)
{
    return wal_append_record(wal, WAL_OP_SET, key, value, 0);
}

bool wal_append_set_ttl(Wal *wal, const char *key, const char *value, int64_t expire_at_ms)
{
    return wal_append_record(wal, WAL_OP_SET_TTL, key, value, expire_at_ms);
}

bool wal_append_del(Wal *wal, const char *key)
{
    return wal_append_record(wal, WAL_OP_DEL, key, NULL, 0);
}

size_t wal_replay(KVStore *kv, const char *filepath)
{
    if (!kv || !filepath) return 0;

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) return 0;

    size_t replayed = 0;
    int64_t now_ms = ttl_current_time_ms();

    while (1) {
        WalHeader hdr;
        ssize_t n = read(fd, &hdr, sizeof(WalHeader));
        if (n == 0) break;
        if (n < (ssize_t)sizeof(WalHeader)) break;

        uint16_t key_len = ntohs(hdr.key_len);
        uint32_t val_len = ntohl(hdr.val_len);
        size_t extra_len = (hdr.op == WAL_OP_SET_TTL) ? sizeof(uint64_t) : 0;

        size_t payload_len = sizeof(WalHeader) + key_len + val_len + extra_len;
        uint8_t *buf = malloc(payload_len);
        if (!buf) break;

        memcpy(buf, &hdr, sizeof(WalHeader));

        ssize_t r_key = read(fd, buf + sizeof(WalHeader), key_len);
        if (r_key < (ssize_t)key_len) { free(buf); break; }

        if (val_len > 0) {
            ssize_t r_val = read(fd, buf + sizeof(WalHeader) + key_len, val_len);
            if (r_val < (ssize_t)val_len) { free(buf); break; }
        }

        if (extra_len > 0) {
            ssize_t r_exp = read(fd, buf + sizeof(WalHeader) + key_len + val_len, extra_len);
            if (r_exp < (ssize_t)extra_len) { free(buf); break; }
        }

        uint32_t disk_crc_net;
        ssize_t r_crc = read(fd, &disk_crc_net, sizeof(disk_crc_net));
        if (r_crc < (ssize_t)sizeof(disk_crc_net)) { free(buf); break; }

        uint32_t expected_crc = wal_crc32(buf, payload_len);
        uint32_t actual_crc   = ntohl(disk_crc_net);

        if (expected_crc != actual_crc) {
            free(buf);
            break;
        }

        char *key = malloc(key_len + 1);
        memcpy(key, buf + sizeof(WalHeader), key_len);
        key[key_len] = '\0';

        if (hdr.op == WAL_OP_SET) {
            char *val = malloc(val_len + 1);
            if (val_len > 0) memcpy(val, buf + sizeof(WalHeader) + key_len, val_len);
            val[val_len] = '\0';

            ht_set(kv->table, key, val);
            free(val);
        } else if (hdr.op == WAL_OP_SET_TTL) {
            char *val = malloc(val_len + 1);
            if (val_len > 0) memcpy(val, buf + sizeof(WalHeader) + key_len, val_len);
            val[val_len] = '\0';

            uint64_t exp_net;
            memcpy(&exp_net, buf + sizeof(WalHeader) + key_len + val_len, sizeof(uint64_t));
            int64_t exp_ms = (int64_t)be64_to_host(exp_net);

            if (exp_ms > 0 && now_ms >= exp_ms) {
                ht_del(kv->table, key);
            } else {
                ht_set_ttl(kv->table, key, val, exp_ms);
            }
            free(val);
        } else if (hdr.op == WAL_OP_DEL) {
            ht_del(kv->table, key);
        }

        free(key);
        free(buf);
        replayed++;
    }

    close(fd);
    return replayed;
}
