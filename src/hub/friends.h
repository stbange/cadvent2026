/* SPDX-License-Identifier: MIT */
#ifndef CADV_HUB_FRIENDS_H
#define CADV_HUB_FRIENDS_H

#include <stddef.h>

#include "hub/progress.h"

/* The crew whose gamer tags are hidden throughout the portals. Names are only
 * ever shown as text. They come from assets/text/friends.cfg so another group
 * can swap in their own names with a one-file change. */

#define CADV_MAX_FRIENDS 12
#define CADV_FRIEND_NAME 32

typedef struct cadv_friends {
    char name[CADV_MAX_FRIENDS][CADV_FRIEND_NAME];
    const char *ptr[CADV_MAX_FRIENDS]; /* convenience pointers into name[] */
    int count;
} cadv_friends;

void cadv_friends_default(cadv_friends *f);
/* Loads "friend.0".."friend.N" from a kv file; falls back to defaults on error.
 * Returns true if the file was used. */
bool cadv_friends_load(cadv_friends *f, const char *path);
/* Formats a mark's text with the friend's name (upper-cased). */
void cadv_friends_mark_text(const cadv_friends *f, const cadv_mark_def *m, char *out, size_t cap);
/* Found / total marks of one friend across all portals. */
void cadv_friends_tally(const cadv_progress *pr, int friend_index, int *found, int *total);

#endif
