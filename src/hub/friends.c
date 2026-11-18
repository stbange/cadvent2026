/* SPDX-License-Identifier: MIT */
#include "hub/friends.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "core/kvstore.h"

static const char *const k_default[] = {"Dr. Doom", "Bloodmaker", "C6P", "X-Ray Junkie", "Messerjocke", "Foen-X"};

static void finish(cadv_friends *f)
{
    int i;
    for (i = 0; i < CADV_MAX_FRIENDS; ++i) {
        f->ptr[i] = f->name[i];
    }
}

void cadv_friends_default(cadv_friends *f)
{
    int i;
    memset(f, 0, sizeof *f);
    f->count = (int)(sizeof k_default / sizeof k_default[0]);
    for (i = 0; i < f->count; ++i) {
        snprintf(f->name[i], CADV_FRIEND_NAME, "%s", k_default[i]);
    }
    finish(f);
}

bool cadv_friends_load(cadv_friends *f, const char *path)
{
    cadv_kvstore *kv = NULL;
    static cadv_kvstore store; /* large; not re-entrant, loaded once at startup */
    int i;
    cadv_friends_default(f);
    kv = &store;
    if (!path || !cadv_kv_load_file(kv, path, "cadvent-friends")) {
        return false;
    }
    for (i = 0; i < CADV_MAX_FRIENDS; ++i) {
        char key[16];
        const char *v;
        snprintf(key, sizeof key, "friend.%d", i);
        v = cadv_kv_get(kv, key);
        if (!v || !*v) {
            break;
        }
        snprintf(f->name[i], CADV_FRIEND_NAME, "%s", v);
    }
    if (i == 0) {
        cadv_friends_default(f);
        return false;
    }
    f->count = i;
    finish(f);
    return true;
}

void cadv_friends_mark_text(const cadv_friends *f, const cadv_mark_def *m, char *out, size_t cap)
{
    char upper[CADV_FRIEND_NAME];
    const char *name = (m->friend_index >= 0 && m->friend_index < f->count) ? f->name[m->friend_index] : "SOMEONE";
    const char *pct;
    size_t i;
    for (i = 0; name[i] && i + 1 < sizeof upper; ++i) {
        upper[i] = (char)toupper((unsigned char)name[i]);
    }
    upper[i] = '\0';
    pct = strstr(m->text, "%s");
    if (!pct) {
        snprintf(out, cap, "%s", m->text);
        return;
    }
    snprintf(out, cap, "%.*s%s%s", (int)(pct - m->text), m->text, upper, pct + 2);
}

void cadv_friends_tally(const cadv_progress *pr, int friend_index, int *found, int *total)
{
    int day;
    *found = 0;
    *total = 0;
    for (day = 1; day <= CADV_PORTAL_COUNT; ++day) {
        const cadv_portal_desc *d = cadv_portal_get(day);
        int i;
        for (i = 0; d && i < d->mark_count; ++i) {
            if (d->marks[i].friend_index == friend_index) {
                ++*total;
                if (pr->p[day - 1].marks_found & (1u << (unsigned)i)) {
                    ++*found;
                }
            }
        }
    }
}
