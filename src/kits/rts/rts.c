/* SPDX-License-Identifier: MIT */
#include "kits/rts/rts.h"

#include <string.h>

#include "core/hash.h"
#include "core/mathx.h"

bool cadv_rts_init(cadv_rts *r, int w, int h, float cell, const cadv_rts_bdef *bdefs, int nbdefs,
                   const cadv_rts_udef *udefs, int nudefs)
{
    int s;
    memset(r, 0, sizeof *r);
    if (!cadv_rts_grid_init(&r->grid, w, h, cell)) {
        return false;
    }
    r->bdefs = bdefs;
    r->nbdefs = nbdefs;
    r->udefs = udefs;
    r->nudefs = nudefs;
    r->last_lost_unit = -1;
    for (s = 0; s < CADV_RTS_SIDES; ++s) {
        r->side[s].ready_building = -1;
        r->side[s].rally_x = -1;
        r->side[s].rally_z = -1;
    }
    return true;
}

void cadv_rts_update_power(cadv_rts *r, int side)
{
    int i;
    cadv_rts_side *sd = &r->side[side];
    bool was_ok = cadv_rts_power_factor(r, side) >= 1.0f;
    sd->power_made = 0;
    sd->power_used = 0;
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        const cadv_rts_building *b = &r->b[i];
        if (b->alive && b->owner == side) {
            int p = r->bdefs[b->def].power;
            if (p > 0) {
                sd->power_made += p;
            } else {
                sd->power_used -= p;
            }
        }
    }
    if (was_ok && cadv_rts_power_factor(r, side) < 1.0f) {
        r->events[side] |= CADV_RTS_EV_LOW_POWER;
    }
}

float cadv_rts_power_factor(const cadv_rts *r, int side)
{
    const cadv_rts_side *sd = &r->side[side];
    float f;
    if (sd->power_used <= sd->power_made) {
        return 1.0f;
    }
    f = (float)sd->power_made / (float)sd->power_used;
    return f < 0.25f ? 0.25f : f;
}

bool cadv_rts_defences_online(const cadv_rts *r, int side) { return cadv_rts_power_factor(r, side) >= 1.0f; }

int cadv_rts_add_building(cadv_rts *r, int owner, int def, int x, int z)
{
    int i;
    const cadv_rts_bdef *d = &r->bdefs[def];
    if (cadv_rts_can_place(&r->grid, x, z, d->fw, d->fh, owner, 0, true) != CADV_PLACE_OK) {
        return -1;
    }
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        if (!r->b[i].alive) {
            cadv_rts_building *b = &r->b[i];
            memset(b, 0, sizeof *b);
            b->alive = true;
            b->def = def;
            b->owner = owner;
            b->x = x;
            b->z = z;
            b->hp = d->hp;
            cadv_rts_place(&r->grid, x, z, d->fw, d->fh, owner, i);
            cadv_rts_update_power(r, owner);
            return i;
        }
    }
    return -1;
}

void cadv_rts_destroy_building(cadv_rts *r, int i)
{
    cadv_rts_building *b = &r->b[i];
    if (!b->alive) {
        return;
    }
    b->alive = false;
    cadv_rts_remove(&r->grid, i);
    cadv_rts_update_power(r, b->owner);
    r->events[b->owner] |= CADV_RTS_EV_BUILDING_LOST;
}

int cadv_rts_add_unit(cadv_rts *r, int owner, int def, float x, float z)
{
    int i;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (!r->u[i].alive) {
            cadv_rts_unit *u = &r->u[i];
            memset(u, 0, sizeof *u);
            u->alive = true;
            u->def = def;
            u->owner = owner;
            u->x = x;
            u->z = z;
            u->hp = r->udefs[def].hp;
            u->target = -1;
            u->carrier = -1;
            u->res_cell.x = -1;
            u->res_cell.z = -1;
            if (r->udefs[def].harvester) {
                u->order = CADV_ORDER_HARVEST;
                u->hstate = CADV_HARV_SEEK;
            }
            return i;
        }
    }
    return -1;
}

static bool has_flag(const cadv_rts *r, int side, uint32_t flag, int *which)
{
    int i;
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        if (r->b[i].alive && r->b[i].owner == side && (r->bdefs[r->b[i].def].flags & flag)) {
            if (which) {
                *which = i;
            }
            return true;
        }
    }
    return false;
}

static uint32_t trained_at(const cadv_rts *r, int udef)
{
    return r->udefs[udef].from ? r->udefs[udef].from : CADV_BF_FACTORY;
}

bool cadv_rts_can_train(const cadv_rts *r, int side, int udef) { return has_flag(r, side, trained_at(r, udef), NULL); }

bool cadv_rts_queue(cadv_rts *r, int side, bool building, int def)
{
    cadv_rts_side *sd = &r->side[side];
    int k = building ? 0 : 1;
    int cost = building ? r->bdefs[def].cost : r->udefs[def].cost;
    int lumber = building ? r->bdefs[def].lumber : r->udefs[def].lumber;
    int tallow = building ? r->bdefs[def].tallow : r->udefs[def].tallow;
    if (sd->queued[k] >= CADV_RTS_QUEUE || (!building && !cadv_rts_can_train(r, side, def))) {
        return false;
    }
    if (sd->credits < cost || sd->lumber < lumber || sd->tallow < tallow) {
        r->events[side] |= CADV_RTS_EV_BROKE;
        return false;
    }
    if (!building && r->use_supply) {
        int queued = 0;
        int i;
        cadv_rts_update_supply(r, side);
        for (i = 0; i < sd->queued[1]; ++i) {
            queued += r->udefs[sd->queue[1][i].def].supply;
        }
        if (sd->supply_used + queued + r->udefs[def].supply > sd->supply_cap) {
            r->events[side] |= CADV_RTS_EV_SUPPLY;
            return false;
        }
    }
    sd->credits -= cost;
    sd->lumber -= lumber;
    sd->tallow -= tallow;
    sd->queue[k][sd->queued[k]].building = building;
    sd->queue[k][sd->queued[k]].def = def;
    sd->queue[k][sd->queued[k]].progress = 0;
    ++sd->queued[k];
    return true;
}

bool cadv_rts_cancel(cadv_rts *r, int side, bool building)
{
    cadv_rts_side *sd = &r->side[side];
    int k = building ? 0 : 1;
    const cadv_rts_queue_item *q;
    if (sd->queued[k] == 0) {
        return false;
    }
    q = &sd->queue[k][--sd->queued[k]];
    sd->credits += q->building ? r->bdefs[q->def].cost : r->udefs[q->def].cost;
    sd->lumber += q->building ? r->bdefs[q->def].lumber : r->udefs[q->def].lumber;
    sd->tallow += q->building ? r->bdefs[q->def].tallow : r->udefs[q->def].tallow;
    return true;
}

void cadv_rts_capture(cadv_rts *r, int building, int side)
{
    cadv_rts_building *b = &r->b[building];
    int old = b->owner;
    int i;
    if (!b->alive || old == side) {
        return;
    }
    b->owner = side;
    for (i = 0; i < r->grid.w * r->grid.h; ++i) {
        if (r->grid.building[i] == building) {
            r->grid.owner[i] = (int8_t)side;
        }
    }
    cadv_rts_update_power(r, old);
    cadv_rts_update_power(r, side);
}

int cadv_rts_building_at(const cadv_rts *r, int x, int z)
{
    return cadv_rts_in_bounds(&r->grid, x, z) ? r->grid.building[cadv_rts_index(&r->grid, x, z)] : -1;
}

void cadv_rts_reveal(cadv_rts *r, int side, float x, float z, int radius)
{
    cadv_cell c = cadv_rts_cell_at(&r->grid, x, z);
    int dx;
    int dz;
    for (dz = -radius; dz <= radius; ++dz) {
        for (dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dz * dz <= radius * radius && cadv_rts_in_bounds(&r->grid, c.x + dx, c.z + dz)) {
                int i = cadv_rts_index(&r->grid, c.x + dx, c.z + dz);
                r->visible[side][i] = 1;
                r->explored[side][i] = 1;
            }
        }
    }
}

void cadv_rts_update_fog(cadv_rts *r, int side)
{
    int i;
    memset(r->visible[side], 0, sizeof r->visible[side]);
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        if (u->alive && u->owner == side) {
            int s = r->udefs[u->def].sight;
            cadv_rts_reveal(r, side, u->x, u->z, s > 0 ? s : 5);
        }
    }
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        const cadv_rts_building *b = &r->b[i];
        if (b->alive && b->owner == side) {
            const cadv_rts_bdef *d = &r->bdefs[b->def];
            cadv_rts_reveal(r, side, ((float)b->x + 0.5f * (float)d->fw) * r->grid.size,
                            ((float)b->z + 0.5f * (float)d->fh) * r->grid.size, d->sight > 0 ? d->sight : 5);
        }
    }
}

bool cadv_rts_cell_visible(const cadv_rts *r, int side, float x, float z)
{
    cadv_cell c = cadv_rts_cell_at(&r->grid, x, z);
    return cadv_rts_in_bounds(&r->grid, c.x, c.z) && r->visible[side][cadv_rts_index(&r->grid, c.x, c.z)];
}

cadv_place_result cadv_rts_place_ready(cadv_rts *r, int side, int x, int z)
{
    cadv_rts_side *sd = &r->side[side];
    const cadv_rts_bdef *d;
    cadv_place_result res;
    if (sd->ready_building < 0) {
        return CADV_PLACE_BLOCKED;
    }
    d = &r->bdefs[sd->ready_building];
    res = cadv_rts_can_place(&r->grid, x, z, d->fw, d->fh, side, 3, false);
    if (res == CADV_PLACE_OK && cadv_rts_add_building(r, side, sd->ready_building, x, z) >= 0) {
        sd->ready_building = -1;
    }
    return res;
}

/* ---- orders ---------------------------------------------------------------------- */

static cadv_cell unit_cell(const cadv_rts *r, const cadv_rts_unit *u) { return cadv_rts_cell_at(&r->grid, u->x, u->z); }

static void plan(cadv_rts *r, cadv_rts_unit *u, cadv_cell goal)
{
    bool naval = r->udefs[u->def].naval;
    int n = cadv_rts_path_for(&r->grid, unit_cell(r, u), goal, u->path, CADV_RTS_UNIT_PATH, naval);
    if (n < 0 && naval && !cadv_rts_passable_for(&r->grid, goal.x, goal.z, true)) {
        /* a boat sent ashore heads for the water closest to that point */
        int ring;
        for (ring = 1; ring <= 5 && n < 0; ++ring) {
            int dx;
            int dz;
            int best = -1;
            cadv_cell bc = goal;
            for (dz = -ring; dz <= ring; ++dz) {
                for (dx = -ring; dx <= ring; ++dx) {
                    if (cadv_rts_passable_for(&r->grid, goal.x + dx, goal.z + dz, true) && (best < 0 || dx * dx + dz * dz < best)) {
                        best = dx * dx + dz * dz;
                        bc.x = goal.x + dx;
                        bc.z = goal.z + dz;
                    }
                }
            }
            if (best >= 0) {
                n = cadv_rts_path_for(&r->grid, unit_cell(r, u), bc, u->path, CADV_RTS_UNIT_PATH, true);
            }
        }
    }
    u->goal = goal;
    u->path_pos = 0;
    u->path_len = n < 0 ? 0 : n;
    u->repath = n < 0 ? 30 : 0;
}

void cadv_rts_order_move(cadv_rts *r, int unit, cadv_cell goal, bool attack_move)
{
    cadv_rts_unit *u = &r->u[unit];
    u->order = attack_move ? CADV_ORDER_ATTACK_MOVE : CADV_ORDER_MOVE;
    u->target = -1;
    plan(r, u, goal);
}

void cadv_rts_order_attack(cadv_rts *r, int unit, int target)
{
    cadv_rts_unit *u = &r->u[unit];
    u->order = CADV_ORDER_ATTACK;
    u->target = target;
    u->path_len = 0;
    u->repath = 0;
}

void cadv_rts_order_stop(cadv_rts *r, int unit)
{
    cadv_rts_unit *u = &r->u[unit];
    u->order = CADV_ORDER_IDLE;
    u->target = -1;
    u->path_len = 0;
}

void cadv_rts_order_harvest(cadv_rts *r, int unit)
{
    cadv_rts_unit *u = &r->u[unit];
    u->order = CADV_ORDER_HARVEST;
    u->hstate = CADV_HARV_SEEK;
    u->path_len = 0;
}

void cadv_rts_group_set(cadv_rts *r, int side, int group, const int *units, int count)
{
    int i;
    memset(r->side[side].group[group], 0, sizeof r->side[side].group[group]);
    for (i = 0; i < count; ++i) {
        if (units[i] >= 0 && units[i] < CADV_RTS_MAX_UNITS && r->u[units[i]].alive && r->u[units[i]].owner == side) {
            r->side[side].group[group][units[i]] = 1;
        }
    }
}

int cadv_rts_group_move(cadv_rts *r, int side, int group, cadv_cell goal, bool attack_move)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (r->side[side].group[group][i] && r->u[i].alive) {
            cadv_rts_order_move(r, i, goal, attack_move);
            ++n;
        }
    }
    return n;
}

/* ---- simulation -------------------------------------------------------------------- */

static void spawn_from_factory(cadv_rts *r, int side, int def)
{
    int f = -1;
    const cadv_rts_bdef *d;
    int x;
    int z;
    has_flag(r, side, trained_at(r, def), &f);
    d = &r->bdefs[r->b[f].def];
    /* first passable cell on the ring around the footprint, south side first */
    for (z = r->b[f].z + d->fh; z >= r->b[f].z - 1; --z) {
        for (x = r->b[f].x - 1; x <= r->b[f].x + d->fw; ++x) {
            if (cadv_rts_passable_for(&r->grid, x, z, r->udefs[def].naval)) {
                cadv_cell c = {x, z};
                float wx;
                float wz;
                int u;
                cadv_rts_cell_centre(&r->grid, c, &wx, &wz);
                u = cadv_rts_add_unit(r, side, def, wx, wz);
                if (u >= 0) {
                    ++r->side[side].units_built;
                    r->events[side] |= CADV_RTS_EV_UNIT_READY;
                    if (r->side[side].rally_x >= 0 && !r->udefs[def].harvester) {
                        cadv_cell g = {r->side[side].rally_x, r->side[side].rally_z};
                        cadv_rts_order_move(r, u, g, true);
                    }
                }
                return;
            }
        }
    }
}

static void tick_queue(cadv_rts *r, int side, int k)
{
    cadv_rts_side *sd = &r->side[side];
    cadv_rts_queue_item *q;
    int need;
    int i;
    if (sd->queued[k] == 0) {
        return;
    }
    q = &sd->queue[k][0];
    need = (q->building ? r->bdefs[q->def].build_ticks : r->udefs[q->def].build_ticks) * 100;
    if (q->progress < need) {
        q->progress += (int)(100.0f * cadv_rts_power_factor(r, side));
        if (q->progress < need) {
            return;
        }
    }
    if (q->building) {
        if (sd->ready_building >= 0) {
            return; /* wait until the last one is placed */
        }
        sd->ready_building = q->def;
        r->events[side] |= CADV_RTS_EV_BUILT;
    } else {
        if (!cadv_rts_can_train(r, side, q->def)) {
            return;
        }
        spawn_from_factory(r, side, q->def);
    }
    for (i = 1; i < sd->queued[k]; ++i) {
        sd->queue[k][i - 1] = sd->queue[k][i];
    }
    --sd->queued[k];
}

static bool follow_path(cadv_rts *r, cadv_rts_unit *u)
{
    float speed = r->udefs[u->def].speed;
    float wx;
    float wz;
    float dx;
    float dz;
    float d;
    cadv_cell next;
    if (u->repath > 0) {
        if (--u->repath > 0) {
            return false; /* waiting to retry an unreachable goal */
        }
        plan(r, u, u->goal);
        if (u->repath > 0) {
            return false;
        }
    }
    if (u->path_pos >= u->path_len) {
        return true;
    }
    next = u->path[u->path_pos];
    if (!cadv_rts_passable_for(&r->grid, next.x, next.z, r->udefs[u->def].naval)) {
        plan(r, u, u->goal); /* something was built in the way */
        return u->path_len == 0 && u->repath == 0;
    }
    cadv_rts_cell_centre(&r->grid, next, &wx, &wz);
    dx = wx - u->x;
    dz = wz - u->z;
    d = cadv_sqrtf(dx * dx + dz * dz);
    if (d <= speed) {
        u->x = wx;
        u->z = wz;
        ++u->path_pos;
    } else {
        u->x += dx / d * speed;
        u->z += dz / d * speed;
        u->yaw = cadv_atan2f(-dx, -dz);
    }
    return u->path_pos >= u->path_len;
}

static bool target_pos(const cadv_rts *r, int target, int owner, float *x, float *z, float *pad)
{
    if (target >= CADV_RTS_TARGET_BUILDING) {
        const cadv_rts_building *b = &r->b[target - CADV_RTS_TARGET_BUILDING];
        const cadv_rts_bdef *d;
        if (!b->alive || b->owner == owner) {
            return false;
        }
        d = &r->bdefs[b->def];
        *x = ((float)b->x + 0.5f * (float)d->fw) * r->grid.size;
        *z = ((float)b->z + 0.5f * (float)d->fh) * r->grid.size;
        *pad = 0.5f * (float)(d->fw > d->fh ? d->fw : d->fh) * r->grid.size;
        return true;
    }
    if (target >= 0 && target < CADV_RTS_MAX_UNITS && r->u[target].alive && r->u[target].owner != owner && r->u[target].carrier < 0) {
        *x = r->u[target].x;
        *z = r->u[target].z;
        *pad = 0.0f;
        return true;
    }
    return false;
}

static int nearest_enemy(const cadv_rts *r, int owner, float x, float z, float range)
{
    int i;
    int best = -1;
    float bd = range;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *e = &r->u[i];
        if (e->alive && e->owner != owner && e->owner != CADV_RTS_NEUTRAL && e->carrier < 0) {
            float d = cadv_sqrtf((e->x - x) * (e->x - x) + (e->z - z) * (e->z - z));
            if (d <= bd) {
                bd = d;
                best = i;
            }
        }
    }
    if (best >= 0) {
        return best;
    }
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        float tx;
        float tz;
        float pad;
        if (r->b[i].owner != CADV_RTS_NEUTRAL && target_pos(r, CADV_RTS_TARGET_BUILDING + i, owner, &tx, &tz, &pad)) {
            float d = cadv_sqrtf((tx - x) * (tx - x) + (tz - z) * (tz - z)) - pad;
            if (d <= bd) {
                bd = d;
                best = CADV_RTS_TARGET_BUILDING + i;
            }
        }
    }
    return best;
}

static void log_shot(cadv_rts *r, int owner, float x0, float z0, int target)
{
    cadv_rts_shot *s = &r->shots[r->shot_next];
    float pad;
    r->shot_next = (r->shot_next + 1) % CADV_RTS_SHOTS;
    s->owner = owner;
    s->x0 = x0;
    s->z0 = z0;
    s->ttl = 8;
    if (!target_pos(r, target, owner, &s->x1, &s->z1, &pad)) {
        s->x1 = x0;
        s->z1 = z0;
    }
}

void cadv_rts_damage(cadv_rts *r, int target, int amount)
{
    if (target >= CADV_RTS_TARGET_BUILDING) {
        cadv_rts_building *b = &r->b[target - CADV_RTS_TARGET_BUILDING];
        b->hp -= amount;
        if (b->hp <= 0) {
            cadv_rts_destroy_building(r, target - CADV_RTS_TARGET_BUILDING);
        }
    } else {
        cadv_rts_unit *u = &r->u[target];
        u->hp -= amount;
        if (u->hp <= 0) {
            int g;
            u->alive = false;
            r->side[u->owner].units_lost++;
            r->events[u->owner] |= CADV_RTS_EV_UNIT_LOST;
            r->last_lost_unit = target;
            for (g = 0; g < CADV_RTS_GROUPS; ++g) {
                r->side[u->owner].group[g][target] = 0;
            }
        }
    }
}

/* Shoots 'target' if it is in range; returns false when it is out of range. */
static bool engage(cadv_rts *r, cadv_rts_unit *u, int target)
{
    const cadv_rts_udef *d = &r->udefs[u->def];
    float tx;
    float tz;
    float pad;
    float dist;
    if (!target_pos(r, target, u->owner, &tx, &tz, &pad)) {
        return true; /* gone: nothing more to do */
    }
    if (target >= CADV_RTS_TARGET_BUILDING) {
        /* measure to the footprint's edge, so diagonal neighbours can strike */
        const cadv_rts_building *b = &r->b[target - CADV_RTS_TARGET_BUILDING];
        const cadv_rts_bdef *bd = &r->bdefs[b->def];
        float x0 = (float)b->x * r->grid.size;
        float z0 = (float)b->z * r->grid.size;
        float ex = cadv_maxf(cadv_maxf(x0 - u->x, u->x - (x0 + (float)bd->fw * r->grid.size)), 0.0f);
        float ez = cadv_maxf(cadv_maxf(z0 - u->z, u->z - (z0 + (float)bd->fh * r->grid.size)), 0.0f);
        dist = cadv_sqrtf(ex * ex + ez * ez);
        (void)pad;
    } else {
        dist = cadv_sqrtf((tx - u->x) * (tx - u->x) + (tz - u->z) * (tz - u->z)) - pad;
    }
    if (dist > d->range) {
        return false;
    }
    u->yaw = cadv_atan2f(-(tx - u->x), -(tz - u->z));
    if (u->cooldown <= 0) {
        log_shot(r, u->owner, u->x, u->z, target);
        cadv_rts_damage(r, target, d->damage);
        u->cooldown = d->cooldown;
        r->events[u->owner] |= CADV_RTS_EV_SHOT;
    }
    return true;
}

static bool adjacent_to(const cadv_rts *r, const cadv_rts_unit *u, int bi)
{
    cadv_cell c = unit_cell(r, u);
    const cadv_rts_building *b = &r->b[bi];
    const cadv_rts_bdef *d = &r->bdefs[b->def];
    return c.x >= b->x - 1 && c.x <= b->x + d->fw && c.z >= b->z - 1 && c.z <= b->z + d->fh;
}

static int nearest_refinery(const cadv_rts *r, const cadv_rts_unit *u)
{
    int i;
    int best = -1;
    float bd = 1e9f;
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        const cadv_rts_building *b = &r->b[i];
        if (b->alive && b->owner == u->owner && (r->bdefs[b->def].flags & CADV_BF_REFINERY)) {
            float x = ((float)b->x + 0.5f * (float)r->bdefs[b->def].fw) * r->grid.size;
            float z = ((float)b->z + 0.5f * (float)r->bdefs[b->def].fh) * r->grid.size;
            float d = (x - u->x) * (x - u->x) + (z - u->z) * (z - u->z);
            if (d < bd) {
                bd = d;
                best = i;
            }
        }
    }
    return best;
}

static bool near_cell(cadv_cell a, cadv_cell b) { return a.x - b.x <= 1 && b.x - a.x <= 1 && a.z - b.z <= 1 && b.z - a.z <= 1; }

static void tick_harvester(cadv_rts *r, int idx)
{
    cadv_rts_unit *u = &r->u[idx];
    const cadv_rts_udef *d = &r->udefs[u->def];
    cadv_cell c = unit_cell(r, u);
    int ref = -1;
    switch (u->hstate) {
    case CADV_HARV_SEEK: {
        cadv_cell ore;
        bool have = u->res_cell.x >= 0 && r->grid.ore[cadv_rts_index(&r->grid, u->res_cell.x, u->res_cell.z)] > 0 &&
                    r->grid.ore_kind[cadv_rts_index(&r->grid, u->res_cell.x, u->res_cell.z)] == u->res_kind;
        if (have) {
            ore = u->res_cell;
        } else if (u->cargo < d->carry && cadv_rts_nearest_resource(&r->grid, c.x, c.z, u->res_kind, &ore)) {
            have = true;
            u->res_cell = ore;
        }
        if (u->cargo >= d->carry || !have) {
            u->hstate = u->cargo > 0 ? CADV_HARV_RETURN : CADV_HARV_SEEK;
            u->path_len = 0;
            break;
        }
        plan(r, u, ore);
        u->hstate = CADV_HARV_TO_ORE;
        break;
    }
    case CADV_HARV_TO_ORE:
        if (follow_path(r, u)) {
            /* work the cell from on top of it, or from beside it for trees and mines */
            bool ok = u->res_cell.x >= 0 && near_cell(c, u->res_cell) &&
                      r->grid.ore[cadv_rts_index(&r->grid, u->res_cell.x, u->res_cell.z)] > 0;
            u->hstate = ok ? CADV_HARV_MINING : CADV_HARV_SEEK;
            if (!ok) {
                u->res_cell.x = -1;
            }
            u->timer = 30;
        }
        break;
    case CADV_HARV_MINING:
        if (--u->timer <= 0) {
            int i = cadv_rts_index(&r->grid, u->res_cell.x, u->res_cell.z);
            int got;
            u->cargo_kind = r->grid.ore_kind[i];
            got = cadv_rts_mine(&r->grid, u->res_cell.x, u->res_cell.z, d->carry / 10 > 0 ? d->carry / 10 : 1);
            if (got > 0 && r->grid.ore[i] == 0) {
                r->events[u->owner] |= CADV_RTS_EV_FELLED;
            }
            u->cargo += got;
            u->timer = 30;
            if (u->cargo >= d->carry) {
                u->hstate = CADV_HARV_RETURN;
                u->path_len = 0;
            } else if (got == 0) {
                u->hstate = CADV_HARV_SEEK; /* cell exhausted: find more */
                u->res_cell.x = -1;
            }
        }
        break;
    case CADV_HARV_RETURN:
        ref = nearest_refinery(r, u);
        if (ref < 0) {
            break; /* wait for a refinery */
        }
        if (adjacent_to(r, u, ref)) {
            u->hstate = CADV_HARV_UNLOAD;
            u->timer = 15;
            break;
        }
        if (u->path_len == 0 || u->path_pos >= u->path_len) {
            cadv_cell g = {r->b[ref].x, r->b[ref].z};
            plan(r, u, g);
            if (u->path_len == 0) {
                break;
            }
        }
        follow_path(r, u);
        break;
    default: /* unloading */
        if (--u->timer <= 0) {
            int chunk = d->carry / 10 > 25 ? d->carry / 10 : 25;
            int part = u->cargo < chunk ? u->cargo : chunk;
            cadv_rts_side *sd = &r->side[u->owner];
            u->cargo -= part;
            if (u->cargo_kind == 1) {
                sd->lumber += part;
            } else if (u->cargo_kind == 2) {
                sd->tallow += part;
            } else {
                sd->credits += part;
            }
            sd->harvested += part;
            r->events[u->owner] |= CADV_RTS_EV_UNLOAD;
            u->timer = 15;
            if (u->cargo == 0) {
                u->hstate = CADV_HARV_SEEK;
            }
        }
        break;
    }
}

int cadv_rts_resource(const cadv_rts *r, int side, int kind)
{
    return kind == 1 ? r->side[side].lumber : kind == 2 ? r->side[side].tallow : r->side[side].credits;
}

void cadv_rts_update_supply(cadv_rts *r, int side)
{
    int i;
    cadv_rts_side *sd = &r->side[side];
    sd->supply_used = 0;
    sd->supply_cap = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (r->u[i].alive && r->u[i].owner == side) {
            sd->supply_used += r->udefs[r->u[i].def].supply;
        }
    }
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        if (r->b[i].alive && r->b[i].owner == side) {
            sd->supply_cap += r->bdefs[r->b[i].def].supply;
        }
    }
}

void cadv_rts_order_harvest_at(cadv_rts *r, int unit, cadv_cell cell)
{
    cadv_rts_unit *u = &r->u[unit];
    if (!cadv_rts_in_bounds(&r->grid, cell.x, cell.z)) {
        return;
    }
    u->res_kind = r->grid.ore_kind[cadv_rts_index(&r->grid, cell.x, cell.z)];
    u->res_cell = cell;
    cadv_rts_order_harvest(r, unit);
}

void cadv_rts_order_board(cadv_rts *r, int unit, int transport)
{
    cadv_rts_unit *u = &r->u[unit];
    u->order = CADV_ORDER_BOARD;
    u->target = transport;
    u->path_len = 0;
    u->repath = 0;
}

int cadv_rts_unload(cadv_rts *r, int transport)
{
    cadv_rts_unit *t = &r->u[transport];
    cadv_cell c = unit_cell(r, t);
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        cadv_rts_unit *p = &r->u[i];
        int ring;
        bool placed = false;
        if (!p->alive || p->carrier != transport) {
            continue;
        }
        for (ring = 1; ring <= 3 && !placed; ++ring) {
            int dx;
            int dz;
            for (dz = -ring; dz <= ring && !placed; ++dz) {
                for (dx = -ring; dx <= ring && !placed; ++dx) {
                    if (cadv_rts_passable(&r->grid, c.x + dx, c.z + dz)) {
                        cadv_cell to = {c.x + dx, c.z + dz};
                        cadv_rts_cell_centre(&r->grid, to, &p->x, &p->z);
                        /* spread passengers a little so they do not stack */
                        p->x += (float)((n % 3) - 1) * 0.6f;
                        p->z += (float)((n / 3) % 3 - 1) * 0.6f;
                        p->carrier = -1;
                        p->order = CADV_ORDER_IDLE;
                        --t->passengers;
                        ++n;
                        placed = true;
                    }
                }
            }
        }
    }
    if (n > 0) {
        r->events[t->owner] |= CADV_RTS_EV_UNLOADED;
    }
    return n;
}

static void tick_board(cadv_rts *r, int idx)
{
    cadv_rts_unit *u = &r->u[idx];
    cadv_rts_unit *t = &r->u[u->target];
    float d;
    if (u->target < 0 || !t->alive || t->owner != u->owner || r->udefs[t->def].capacity <= 0) {
        cadv_rts_order_stop(r, idx);
        return;
    }
    d = cadv_sqrtf((t->x - u->x) * (t->x - u->x) + (t->z - u->z) * (t->z - u->z));
    if (d <= 2.3f * r->grid.size) {
        if (t->passengers < r->udefs[t->def].capacity) {
            u->carrier = u->target;
            ++t->passengers;
            u->x = t->x;
            u->z = t->z;
            u->order = CADV_ORDER_IDLE;
        }
        return;
    }
    if (u->path_pos >= u->path_len || r->ticks % 60 == 0) {
        plan(r, u, unit_cell(r, t));
    }
    follow_path(r, u);
}

static void tick_unit(cadv_rts *r, int idx)
{
    cadv_rts_unit *u = &r->u[idx];
    const cadv_rts_udef *d = &r->udefs[u->def];
    if (u->cooldown > 0) {
        --u->cooldown;
    }
    if (u->order == CADV_ORDER_HARVEST) {
        tick_harvester(r, idx);
        return;
    }
    if (u->order == CADV_ORDER_BOARD) {
        tick_board(r, idx);
        return;
    }
    if (d->damage <= 0) {
        if ((u->order == CADV_ORDER_MOVE || u->order == CADV_ORDER_ATTACK_MOVE) && follow_path(r, u)) {
            u->order = CADV_ORDER_IDLE;
            /* a transport sent to a shore unloads there */
            if (d->capacity > 0 && u->passengers > 0 &&
                !cadv_rts_passable_for(&r->grid, u->goal.x, u->goal.z, d->naval)) {
                cadv_rts_unload(r, idx);
            }
        }
        return;
    }
    switch (u->order) {
    case CADV_ORDER_ATTACK: {
        float tx;
        float tz;
        float pad;
        if (!target_pos(r, u->target, u->owner, &tx, &tz, &pad)) {
            cadv_rts_order_stop(r, idx);
            break;
        }
        if (!engage(r, u, u->target)) {
            if (u->path_pos >= u->path_len && u->repath == 0) {
                plan(r, u, cadv_rts_cell_at(&r->grid, tx, tz));
                if (u->path_len == 0 && u->repath == 0) {
                    u->repath = 30;
                }
            }
            follow_path(r, u);
        }
        break;
    }
    case CADV_ORDER_ATTACK_MOVE:
    case CADV_ORDER_IDLE: {
        int e = nearest_enemy(r, u->owner, u->x, u->z, d->range);
        if (e >= 0) {
            engage(r, u, e);
        } else if (u->order == CADV_ORDER_ATTACK_MOVE && follow_path(r, u)) {
            u->order = CADV_ORDER_IDLE;
        }
        break;
    }
    default: /* plain move ignores enemies */
        if (follow_path(r, u)) {
            u->order = CADV_ORDER_IDLE;
        }
        break;
    }
}

static void tick_defence(cadv_rts *r, int i)
{
    cadv_rts_building *b = &r->b[i];
    const cadv_rts_bdef *d = &r->bdefs[b->def];
    float x = ((float)b->x + 0.5f * (float)d->fw) * r->grid.size;
    float z = ((float)b->z + 0.5f * (float)d->fh) * r->grid.size;
    int e;
    if (b->cooldown > 0) {
        --b->cooldown;
        return;
    }
    if (!cadv_rts_defences_online(r, b->owner)) {
        return;
    }
    e = nearest_enemy(r, b->owner, x, z, d->range);
    if (e >= 0 && e < CADV_RTS_TARGET_BUILDING) {
        log_shot(r, b->owner, x, z, e);
        cadv_rts_damage(r, e, d->damage);
        b->cooldown = d->cooldown;
        r->events[b->owner] |= CADV_RTS_EV_SHOT;
    }
}

void cadv_rts_tick(cadv_rts *r)
{
    int s;
    int i;
    ++r->ticks;
    for (s = 0; s < CADV_RTS_SIDES; ++s) {
        r->events[s] = 0;
        tick_queue(r, s, 0);
        tick_queue(r, s, 1);
        if (cadv_rts_power_factor(r, s) < 1.0f && r->ticks % 300 == 1) {
            r->events[s] |= CADV_RTS_EV_LOW_POWER;
        }
    }
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        cadv_rts_unit *u = &r->u[i];
        if (!u->alive) {
            continue;
        }
        if (u->carrier >= 0) {
            const cadv_rts_unit *t = &r->u[u->carrier];
            if (!t->alive) {
                cadv_rts_damage(r, i, u->hp); /* went down with the boat */
            } else {
                u->x = t->x;
                u->z = t->z;
            }
            continue;
        }
        tick_unit(r, i);
    }
    for (s = 0; s < CADV_RTS_SIDES; ++s) {
        cadv_rts_update_supply(r, s);
    }
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        if (r->b[i].alive && (r->bdefs[r->b[i].def].flags & CADV_BF_DEFENCE)) {
            tick_defence(r, i);
        }
    }
    for (i = 0; i < CADV_RTS_SHOTS; ++i) {
        if (r->shots[i].ttl > 0) {
            --r->shots[i].ttl;
        }
    }
    for (s = 0; s < CADV_RTS_SIDES - 1; ++s) {
        cadv_rts_update_fog(r, s);
    }
}

int cadv_rts_count_units(const cadv_rts *r, int side, int def)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        n += r->u[i].alive && r->u[i].owner == side && (def < 0 || r->u[i].def == def);
    }
    return n;
}

int cadv_rts_count_buildings(const cadv_rts *r, int side, int def)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        n += r->b[i].alive && r->b[i].owner == side && (def < 0 || r->b[i].def == def);
    }
    return n;
}

static uint64_t hash_f(uint64_t h, float f)
{
    uint32_t v;
    memcpy(&v, &f, sizeof v);
    return cadv_hash_u32(h, v);
}

uint64_t cadv_rts_hash(const cadv_rts *r)
{
    uint64_t h = CADV_FNV64_OFFSET;
    int i;
    h = cadv_hash_u32(h, (uint32_t)r->ticks);
    for (i = 0; i < CADV_RTS_SIDES; ++i) {
        const cadv_rts_side *s = &r->side[i];
        int q;
        int k;
        h = cadv_hash_u32(h, (uint32_t)s->credits);
        h = cadv_hash_u32(h, (uint32_t)s->lumber);
        h = cadv_hash_u32(h, (uint32_t)s->tallow);
        h = cadv_hash_u32(h, (uint32_t)s->ready_building);
        for (k = 0; k < 2; ++k) {
            h = cadv_hash_u32(h, (uint32_t)s->queued[k]);
            for (q = 0; q < s->queued[k]; ++q) {
                h = cadv_hash_u32(h, (uint32_t)s->queue[k][q].def);
                h = cadv_hash_u32(h, (uint32_t)s->queue[k][q].progress);
            }
        }
    }
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        const cadv_rts_building *b = &r->b[i];
        if (b->alive) {
            h = cadv_hash_u32(h, (uint32_t)i);
            h = cadv_hash_u32(h, (uint32_t)(b->def | (b->owner << 8)));
            h = cadv_hash_u32(h, (uint32_t)(b->x | (b->z << 16)));
            h = cadv_hash_u32(h, (uint32_t)b->hp);
        }
    }
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        if (u->alive) {
            h = cadv_hash_u32(h, (uint32_t)i);
            h = cadv_hash_u32(h, (uint32_t)(u->def | (u->owner << 8) | ((int)u->order << 16) | ((int)u->hstate << 24)));
            h = hash_f(h, u->x);
            h = hash_f(h, u->z);
            h = cadv_hash_u32(h, (uint32_t)u->hp);
            h = cadv_hash_u32(h, (uint32_t)u->cargo);
            h = cadv_hash_u32(h, (uint32_t)(u->carrier + 1));
        }
    }
    return h;
}
