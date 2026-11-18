/* SPDX-License-Identifier: MIT */
#include "core/kvstore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cadv_kv_clear(cadv_kvstore *kv)
{
    kv->count = 0;
    kv->version = 1;
}

static bool key_ok(const char *k)
{
    size_t n = strlen(k);
    size_t i;
    if (n == 0 || n >= CADV_KV_KEY) {
        return false;
    }
    for (i = 0; i < n; ++i) {
        char c = k[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                  c == '_' || c == '.' || c == '-';
        if (!ok) {
            return false;
        }
    }
    return true;
}

static bool val_ok(const char *v)
{
    size_t n = strlen(v);
    size_t i;
    if (n >= CADV_KV_VAL) {
        return false;
    }
    for (i = 0; i < n; ++i) {
        if ((unsigned char)v[i] < 0x20) {
            return false;
        }
    }
    return true;
}

static int find(const cadv_kvstore *kv, const char *key)
{
    int i;
    for (i = 0; i < kv->count; ++i) {
        if (strcmp(kv->e[i].key, key) == 0) {
            return i;
        }
    }
    return -1;
}

bool cadv_kv_set(cadv_kvstore *kv, const char *key, const char *val)
{
    int i;
    if (!key_ok(key) || !val_ok(val)) {
        return false;
    }
    i = find(kv, key);
    if (i < 0) {
        if (kv->count >= CADV_KV_MAX) {
            return false;
        }
        i = kv->count++;
        strcpy(kv->e[i].key, key);
    }
    strcpy(kv->e[i].val, val);
    return true;
}

bool cadv_kv_set_int(cadv_kvstore *kv, const char *key, long long v)
{
    char buf[32];
    snprintf(buf, sizeof buf, "%lld", v);
    return cadv_kv_set(kv, key, buf);
}

const char *cadv_kv_get(const cadv_kvstore *kv, const char *key)
{
    int i = find(kv, key);
    return i < 0 ? NULL : kv->e[i].val;
}

long long cadv_kv_get_int(const cadv_kvstore *kv, const char *key, long long fallback)
{
    const char *v = cadv_kv_get(kv, key);
    char *end = NULL;
    long long r;
    if (!v || !*v) {
        return fallback;
    }
    r = strtoll(v, &end, 10);
    return (*end == '\0') ? r : fallback;
}

bool cadv_kv_remove(cadv_kvstore *kv, const char *key)
{
    int i = find(kv, key);
    if (i < 0) {
        return false;
    }
    kv->e[i] = kv->e[kv->count - 1];
    --kv->count;
    return true;
}

bool cadv_kv_parse(cadv_kvstore *kv, const char *text, const char *magic)
{
    const char *p = text;
    char line[CADV_KV_KEY + CADV_KV_VAL + 8];
    bool first = true;
    cadv_kv_clear(kv);
    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        char *eq;
        if (len > 0 && p[len - 1] == '\r') {
            --len;
        }
        if (len >= sizeof line) {
            return false;
        }
        memcpy(line, p, len);
        line[len] = '\0';
        p = nl ? nl + 1 : p + strlen(p);
        if (first) {
            size_t ml = strlen(magic);
            char *end = NULL;
            long v;
            first = false;
            if (strncmp(line, magic, ml) != 0 || line[ml] != ' ') {
                return false;
            }
            v = strtol(line + ml + 1, &end, 10);
            if (*end != '\0' || v < 1) {
                return false;
            }
            kv->version = (int)v;
            continue;
        }
        if (len == 0 || line[0] == '#') {
            continue;
        }
        eq = strchr(line, '=');
        if (!eq) {
            return false;
        }
        *eq = '\0';
        if (!cadv_kv_set(kv, line, eq + 1)) {
            return false;
        }
    }
    return !first;
}

size_t cadv_kv_serialize(const cadv_kvstore *kv, const char *magic, char *out, size_t cap)
{
    size_t total = 0;
    int i;
    int n = snprintf(out, cap, "%s %d\n", magic, kv->version);
    total += (size_t)n;
    for (i = 0; i < kv->count; ++i) {
        char *dst = (out && total < cap) ? out + total : NULL;
        size_t room = (out && total < cap) ? cap - total : 0;
        n = snprintf(dst, room, "%s=%s\n", kv->e[i].key, kv->e[i].val);
        total += (size_t)n;
    }
    return total;
}

bool cadv_kv_load_file(cadv_kvstore *kv, const char *path, const char *magic)
{
    FILE *f = fopen(path, "rb");
    char *buf;
    long size;
    bool ok;
    cadv_kv_clear(kv);
    if (!f) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0 || size > 1024 * 1024) {
        fclose(f);
        return false;
    }
    buf = (char *)malloc((size_t)size + 1);
    if (!buf) {          /* GCOVR_EXCL_BR_LINE: allocation failure */
        fclose(f);       /* GCOVR_EXCL_LINE */
        return false;    /* GCOVR_EXCL_LINE */
    }
    ok = fread(buf, 1, (size_t)size, f) == (size_t)size;
    fclose(f);
    buf[size] = '\0';
    ok = ok && cadv_kv_parse(kv, buf, magic);
    free(buf);
    return ok;
}

bool cadv_kv_save_file(const cadv_kvstore *kv, const char *path, const char *magic)
{
    size_t need = cadv_kv_serialize(kv, magic, NULL, 0);
    char *buf = (char *)malloc(need + 1);
    FILE *f;
    bool ok;
    if (!buf) {          /* GCOVR_EXCL_BR_LINE: allocation failure */
        return false;    /* GCOVR_EXCL_LINE */
    }
    cadv_kv_serialize(kv, magic, buf, need + 1);
    f = fopen(path, "wb");
    if (!f) {
        free(buf);
        return false;
    }
    ok = fwrite(buf, 1, need, f) == need;
    ok = (fclose(f) == 0) && ok;
    free(buf);
    return ok;
}
