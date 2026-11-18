/* SPDX-License-Identifier: MIT */
#ifndef CADV_CORE_KVSTORE_H
#define CADV_CORE_KVSTORE_H

#include <stdbool.h>
#include <stddef.h>

/* Tiny text key=value store used for save files and config files.
 * Format: first line "<magic> <version>", then "key=value" lines; '#' starts
 * a comment line. Keys: [A-Za-z0-9_.-], values: printable text without newlines. */
#define CADV_KV_MAX 256
#define CADV_KV_KEY 48
#define CADV_KV_VAL 128

typedef struct cadv_kv_entry {
    char key[CADV_KV_KEY];
    char val[CADV_KV_VAL];
} cadv_kv_entry;

typedef struct cadv_kvstore {
    cadv_kv_entry e[CADV_KV_MAX];
    int count;
    int version;
} cadv_kvstore;

void cadv_kv_clear(cadv_kvstore *kv);
bool cadv_kv_set(cadv_kvstore *kv, const char *key, const char *val);
bool cadv_kv_set_int(cadv_kvstore *kv, const char *key, long long v);
const char *cadv_kv_get(const cadv_kvstore *kv, const char *key); /* NULL if absent */
long long cadv_kv_get_int(const cadv_kvstore *kv, const char *key, long long fallback);
bool cadv_kv_remove(cadv_kvstore *kv, const char *key);

/* Returns false on a bad header/magic or malformed line; kv is cleared first. */
bool cadv_kv_parse(cadv_kvstore *kv, const char *text, const char *magic);
/* Returns bytes needed (excluding NUL), snprintf-style. */
size_t cadv_kv_serialize(const cadv_kvstore *kv, const char *magic, char *out, size_t cap);

bool cadv_kv_load_file(cadv_kvstore *kv, const char *path, const char *magic);
bool cadv_kv_save_file(const cadv_kvstore *kv, const char *path, const char *magic);

#endif
