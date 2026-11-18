/* SPDX-License-Identifier: MIT */
#include "kits/fps/map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const k_pickups[] = {"health", "armor", "ammo", "key", "weapon", "star", "jetpack", "fuel"};

int cadv_map_pickup_kind(const char *name)
{
    int i;
    for (i = 0; i < 8; ++i) {
        if (strcmp(name, k_pickups[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static bool err(cadv_map *m, int line, const char *what)
{
    snprintf(m->error, sizeof m->error, "line %d: %s", line, what);
    return false;
}

/* Copies the rest of the line after 'skip' whitespace-separated fields. */
static void rest_of_line(const char *line, int skip, char *out, size_t cap)
{
    const char *p = line;
    int i;
    for (i = 0; i < skip; ++i) {
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        while (*p && *p != ' ' && *p != '\t') {
            ++p;
        }
    }
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    snprintf(out, cap, "%s", p);
}

static cadv_vec3 lo3(const float *v) { return cadv_v3(cadv_minf(v[0], v[3]), cadv_minf(v[1], v[4]), cadv_minf(v[2], v[5])); }
static cadv_vec3 hi3(const float *v) { return cadv_v3(cadv_maxf(v[0], v[3]), cadv_maxf(v[1], v[4]), cadv_maxf(v[2], v[5])); }

static bool parse_line(cadv_map *m, char *line, int ln)
{
    char kw[32] = "";
    char s1[64] = "";
    float v[10];
    int n;
    memset(v, 0, sizeof v);
    if (sscanf(line, "%31s", kw) != 1 || kw[0] == '#') {
        return true;
    }
    if (strcmp(kw, "name") == 0) {
        rest_of_line(line, 1, m->name, sizeof m->name);
    } else if (strcmp(kw, "spawn") == 0) {
        if (sscanf(line, "%*s %f %f %f %f", &v[0], &v[1], &v[2], &v[3]) != 4) {
            return err(m, ln, "spawn needs x y z yaw");
        }
        m->spawn = cadv_v3(v[0], v[1], v[2]);
        m->spawn_yaw = v[3];
    } else if (strcmp(kw, "box") == 0 || strcmp(kw, "deco") == 0 || strcmp(kw, "glass") == 0 ||
               strcmp(kw, "target") == 0 || strcmp(kw, "door") == 0) {
        cadv_map_box *b;
        int kind = kw[0] == 'b' ? 0 : kw[0] == 'g' ? 2 : kw[0] == 't' ? 3 : kw[1] == 'e' ? 1 : 4;
        bool has_value = kind >= 2;
        bool has_mat = kind != 3;
        if (m->nbox >= CADV_MAP_MAX_BOXES) {
            return err(m, ln, "too many boxes");
        }
        b = &m->box[m->nbox];
        memset(b, 0, sizeof *b);
        if (has_value && has_mat) {
            n = sscanf(line, "%*s %f %f %f %f %f %f %f %31s", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], b->mat);
            if (n != 8) {
                return err(m, ln, "expected 6 coordinates, a value and a material");
            }
        } else if (has_value) {
            n = sscanf(line, "%*s %f %f %f %f %f %f %f", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6]);
            snprintf(b->mat, sizeof b->mat, "target");
            if (n != 7) {
                return err(m, ln, "expected 6 coordinates and a value");
            }
        } else {
            n = sscanf(line, "%*s %f %f %f %f %f %f %31s", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], b->mat);
            if (n != 7) {
                return err(m, ln, "expected 6 coordinates and a material");
            }
        }
        b->lo = lo3(v);
        b->hi = hi3(v);
        b->kind = kind;
        b->value = (int)v[6];
        m->nbox++;
    } else {
        cadv_map_item it;
        cadv_map_item *list = NULL;
        int *count = NULL;
        int cap = 0;
        memset(&it, 0, sizeof it);
        if (strcmp(kw, "enemy") == 0) {
            if (sscanf(line, "%*s %31s %f %f %f %f", it.name, &v[0], &v[1], &v[2], &v[3]) != 5) {
                return err(m, ln, "enemy needs TYPE x y z yaw");
            }
            it.pos = cadv_v3(v[0], v[1], v[2]);
            it.yaw = v[3];
            list = m->enemy, count = &m->nenemy, cap = CADV_MAP_MAX_ITEMS;
        } else if (strcmp(kw, "pickup") == 0) {
            if (sscanf(line, "%*s %31s %f %f %f %f %f", it.name, &v[0], &v[1], &v[2], &v[3], &v[4]) != 6 ||
                cadv_map_pickup_kind(it.name) < 0) {
                return err(m, ln, "pickup needs KIND VALUE SUB x y z");
            }
            it.a = v[0];
            it.b = v[1];
            it.pos = cadv_v3(v[2], v[3], v[4]);
            list = m->pickup, count = &m->npickup, cap = CADV_MAP_MAX_ITEMS;
        } else if (strcmp(kw, "prop") == 0 || strcmp(kw, "mark") == 0) {
            if (sscanf(line, "%*s %31s %f %f %f %f", it.name, &v[0], &v[1], &v[2], &v[3]) != 5) {
                return err(m, ln, "expected NAME x y z yaw");
            }
            it.pos = cadv_v3(v[0], v[1], v[2]);
            it.yaw = v[3];
            it.a = (float)atof(it.name);
            if (kw[0] == 'p') {
                list = m->prop, count = &m->nprop, cap = CADV_MAP_MAX_ITEMS;
            } else {
                list = m->mark, count = &m->nmark, cap = 16;
            }
        } else if (strcmp(kw, "secret") == 0 || strcmp(kw, "trigger") == 0) {
            bool trig = kw[0] == 't';
            n = trig ? sscanf(line, "%*s %31s %f %f %f %f %f %f", it.name, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5])
                     : sscanf(line, "%*s %f %f %f %f %f %f", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
            if (n != (trig ? 7 : 6)) {
                return err(m, ln, "volume needs 6 coordinates");
            }
            it.pos = lo3(v);
            it.hi = hi3(v);
            if (trig) {
                list = m->trigger, count = &m->ntrigger, cap = 32;
            } else {
                list = m->secret, count = &m->nsecret, cap = 16;
            }
        } else if (strcmp(kw, "light") == 0) {
            if (sscanf(line, "%*s %f %f %f %f %f %f %f %f", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7]) != 8) {
                return err(m, ln, "light needs x y z r g b radius shadow");
            }
            it.pos = cadv_v3(v[0], v[1], v[2]);
            it.hi = cadv_v3(v[3], v[4], v[5]);
            it.a = v[6];
            it.b = v[7];
            list = m->light, count = &m->nlight, cap = CADV_MAP_MAX_ITEMS;
        } else if (strcmp(kw, "sign") == 0) {
            if (sscanf(line, "%*s %f %f %f %f %f %63s", &v[0], &v[1], &v[2], &v[3], &v[4], s1) != 6) {
                return err(m, ln, "sign needs x y z yaw width TEXT");
            }
            it.pos = cadv_v3(v[0], v[1], v[2]);
            it.yaw = v[3];
            it.a = v[4];
            rest_of_line(line, 6, it.text, sizeof it.text);
            list = m->sign, count = &m->nsign, cap = 32;
        } else if (strcmp(kw, "waypoint") == 0) {
            n = sscanf(line, "%*s %f %f %f %31s", &v[0], &v[1], &v[2], it.name);
            if (n < 3) {
                return err(m, ln, "waypoint needs x y z [ACTION]");
            }
            it.pos = cadv_v3(v[0], v[1], v[2]);
            list = m->waypoint, count = &m->nwaypoint, cap = CADV_MAP_MAX_ITEMS;
        } else {
            return err(m, ln, "unknown keyword");
        }
        if (*count >= cap) {
            return err(m, ln, "too many entries");
        }
        list[(*count)++] = it;
    }
    return true;
}

bool cadv_map_parse(cadv_map *m, const char *text)
{
    char line[256];
    int ln = 0;
    memset(m, 0, sizeof *m);
    while (*text) {
        const char *nl = strchr(text, '\n');
        size_t len = nl ? (size_t)(nl - text) : strlen(text);
        ++ln;
        if (len >= sizeof line) {
            return err(m, ln, "line too long");
        }
        memcpy(line, text, len);
        line[len] = '\0';
        if (len > 0 && line[len - 1] == '\r') {
            line[len - 1] = '\0';
        }
        if (!parse_line(m, line, ln)) {
            return false;
        }
        text += len;
        if (*text == '\n') {
            ++text;
        }
    }
    return true;
}

bool cadv_map_load(cadv_map *m, const char *path)
{
    FILE *f = fopen(path, "rb");
    char *buf;
    long size;
    bool ok;
    if (!f) {
        memset(m, 0, sizeof *m);
        snprintf(m->error, sizeof m->error, "cannot open %s", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (char *)malloc((size_t)(size > 0 ? size : 0) + 1);
    if (!buf) {          /* GCOVR_EXCL_BR_LINE: allocation failure */
        fclose(f);       /* GCOVR_EXCL_LINE */
        return false;    /* GCOVR_EXCL_LINE */
    }
    ok = fread(buf, 1, (size_t)(size > 0 ? size : 0), f) == (size_t)(size > 0 ? size : 0);
    fclose(f);
    buf[size > 0 ? size : 0] = '\0';
    ok = ok && cadv_map_parse(m, buf);
    free(buf);
    return ok;
}

const cadv_map_item *cadv_map_find_trigger(const cadv_map *m, const char *name)
{
    int i;
    for (i = 0; i < m->ntrigger; ++i) {
        if (strcmp(m->trigger[i].name, name) == 0) {
            return &m->trigger[i];
        }
    }
    return NULL;
}

void cadv_map_apply(const cadv_map *m, cadv_fps *f, int (*enemy_type)(const char *name), cadv_map_links *links)
{
    int i;
    for (i = 0; i < m->nbox; ++i) {
        const cadv_map_box *b = &m->box[i];
        switch (b->kind) {
        case 0: links->box_solid[i] = cadv_world_add(&f->world, b->lo, b->hi, 0); break;
        case 2: links->box_solid[i] = cadv_fps_add_breakable(f, b->lo, b->hi, b->value, 0); break;
        case 3: links->box_solid[i] = cadv_fps_add_breakable(f, b->lo, b->hi, b->value, 1); break;
        case 4: links->box_solid[i] = cadv_fps_add_door(f, b->lo, b->hi, (uint32_t)b->value); break;
        default: links->box_solid[i] = -1; break;
        }
    }
    for (i = 0; i < m->nprop; ++i) {
        const cadv_map_item *p = &m->prop[i];
        links->prop_solid[i] = cadv_world_add(&f->world, cadv_v3(p->pos.x - 0.45f, p->pos.y, p->pos.z - 0.45f),
                                              cadv_v3(p->pos.x + 0.45f, p->pos.y + 1.8f, p->pos.z + 0.45f), 400 + i);
    }
    for (i = 0; i < m->nenemy; ++i) {
        int t = enemy_type ? enemy_type(m->enemy[i].name) : -1;
        links->enemy_index[i] = t >= 0 ? cadv_fps_add_enemy(f, t, m->enemy[i].pos, m->enemy[i].yaw) : -1;
    }
    for (i = 0; i < m->npickup; ++i) {
        const cadv_map_item *p = &m->pickup[i];
        links->pickup_index[i] = cadv_fps_add_pickup_ex(f, (cadv_pickup_kind)cadv_map_pickup_kind(p->name),
                                                        (int)p->a, (int)p->b, p->pos);
    }
    cadv_player_spawn(&f->player, m->spawn, m->spawn_yaw);
}
