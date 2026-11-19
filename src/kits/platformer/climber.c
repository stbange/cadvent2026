/* SPDX-License-Identifier: MIT */
#include "kits/platformer/climber.h"

#include <string.h>

static int ifloor(float v)
{
    int i = (int)v;
    return (v < 0.0f && (float)i != v) ? i - 1 : i;
}

void cadv_climber_default_def(cadv_climber_def *d)
{
    d->radius = 0.3f;
    d->height = 1.7f;
    d->eye = 1.6f;
    d->walk = 3.0f;
    d->run = 5.0f;
    d->standing_jump = 3.2f;
    d->running_jump = 4.9f;
    d->jump_vy = 6.0f;
    d->gravity = 20.0f;
    d->swim = 2.6f;
    d->air_max = 30.0f;
}

void cadv_climber_spawn(cadv_climber *c, const cadv_climber_def *d, cadv_vec3 pos, float yaw)
{
    memset(c, 0, sizeof *c);
    c->pos = pos;
    c->yaw = yaw;
    c->air = d->air_max;
    c->peak_y = pos.y;
}

cadv_vec3 cadv_climber_eye(const cadv_climber *c, const cadv_climber_def *d)
{
    return cadv_v3(c->pos.x, c->pos.y + d->eye, c->pos.z);
}

cadv_vec3 cadv_climber_forward(const cadv_climber *c) { return cadv_v3_from_yaw_pitch(c->yaw, c->pitch); }

void cadv_climber_facing(const cadv_climber *c, int *dx, int *dz)
{
    float fx = -cadv_sinf(c->yaw);
    float fz = -cadv_cosf(c->yaw);
    if (cadv_absf(fx) > cadv_absf(fz)) {
        *dx = fx > 0.0f ? 1 : -1;
        *dz = 0;
    } else {
        *dx = 0;
        *dz = fz > 0.0f ? 1 : -1;
    }
}

/* Is the body box (feet raised by 'lift') free of solid tiles? */
static bool box_free(const cadv_tiles *t, float x, float y, float z, float r, float h, float lift)
{
    int x0 = ifloor(x - r + 0.001f);
    int x1 = ifloor(x + r - 0.001f);
    int y0 = ifloor(y + lift);
    int y1 = ifloor(y + h - 0.001f);
    int z0 = ifloor(z - r + 0.001f);
    int z1 = ifloor(z + r - 0.001f);
    int ix;
    int iy;
    int iz;
    for (iy = y0; iy <= y1; ++iy) {
        for (iz = z0; iz <= z1; ++iz) {
            for (ix = x0; ix <= x1; ++ix) {
                if (cadv_tiles_solid_at(t, ix, iy, iz)) {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool supported(const cadv_tiles *t, cadv_vec3 p)
{
    return cadv_tiles_solid_at(t, ifloor(p.x), ifloor(p.y - 0.01f), ifloor(p.z));
}

static bool in_water(const cadv_tiles *t, cadv_vec3 p, float h)
{
    return cadv_tiles_at(t, cadv_v3(p.x, p.y + h, p.z)) == CADV_TILE_WATER;
}

/* Horizontal move with per-axis collision; returns false if blocked on an axis. */
static bool slide(cadv_climber *c, const cadv_climber_def *d, const cadv_tiles *t, float dx, float dz)
{
    bool ok = true;
    if (dx != 0.0f) {
        if (box_free(t, c->pos.x + dx, c->pos.y, c->pos.z, d->radius, d->height, 0.05f)) {
            c->pos.x += dx;
        } else {
            ok = false;
        }
    }
    if (dz != 0.0f) {
        if (box_free(t, c->pos.x, c->pos.y, c->pos.z + dz, d->radius, d->height, 0.05f)) {
            c->pos.z += dz;
        } else {
            ok = false;
        }
    }
    return ok;
}

/* Front cell of the player in grid direction (dx, dz) and the distance from
 * the body to that cell's near face. */
static void front(const cadv_climber *c, const cadv_climber_def *d, int dx, int dz, int *fx, int *fz, float *gap)
{
    int cx = ifloor(c->pos.x);
    int cz = ifloor(c->pos.z);
    *fx = cx + dx;
    *fz = cz + dz;
    if (dx != 0) {
        float face = dx > 0 ? (float)*fx : (float)(*fx + 1);
        *gap = cadv_absf(face - c->pos.x) - d->radius;
    } else {
        float face = dz > 0 ? (float)*fz : (float)(*fz + 1);
        *gap = cadv_absf(face - c->pos.z) - d->radius;
    }
}

static bool ledge_at(const cadv_tiles *t, int fx, int ty, int fz)
{
    return cadv_tiles_solid_at(t, fx, ty - 1, fz) && !cadv_tiles_solid_at(t, fx, ty, fz) &&
           !cadv_tiles_solid_at(t, fx, ty + 1, fz);
}

static void start_move(cadv_climber *c, cadv_climb_state s, cadv_vec3 to, float duration)
{
    c->state = s;
    c->from = c->pos;
    c->to = to;
    c->timer = 0.0f;
    c->duration = duration;
    c->vel = cadv_v3(0, 0, 0);
}

static cadv_vec3 cell_top(int x, int y, int z) { return cadv_v3((float)x + 0.5f, (float)y, (float)z + 0.5f); }

static bool try_grab(cadv_climber *c, const cadv_climber_def *d, const cadv_tiles *t)
{
    int dx;
    int dz;
    int fx;
    int fz;
    float gap;
    float hands = c->pos.y + 1.5f;
    int ty = ifloor(hands + 0.4f);
    cadv_climber_facing(c, &dx, &dz);
    front(c, d, dx, dz, &fx, &fz, &gap);
    if (gap > 0.3f || hands - (float)ty > 0.3f || !ledge_at(t, fx, ty, fz)) {
        return false;
    }
    c->state = CADV_CLIMB_HANG;
    c->dirx = dx;
    c->dirz = dz;
    c->ledge_y = ty;
    c->vel = cadv_v3(0, 0, 0);
    c->pos.y = (float)ty - 1.5f;
    if (dx != 0) {
        c->pos.x = (dx > 0 ? (float)fx : (float)(fx + 1)) - (float)dx * (d->radius + 0.05f);
    } else {
        c->pos.z = (dz > 0 ? (float)fz : (float)(fz + 1)) - (float)dz * (d->radius + 0.05f);
    }
    return true;
}

static void tick_ground(cadv_climber *c, const cadv_climber_def *d, const cadv_input_frame *in, cadv_tiles *t, float dt, uint32_t *ev)
{
    float fx = -cadv_sinf(c->yaw);
    float fz = -cadv_cosf(c->yaw);
    float mx = cadv_clampf(in->move_x, -1, 1);
    float my = cadv_clampf(in->move_y, -1, 1);
    bool run = cadv_input_down(in, CADV_ACT_RUN);
    bool grab = cadv_input_down(in, CADV_ACT_USE);
    float speed = run ? d->run : d->walk;
    float wx = (fx * my - fz * mx);
    float wz = (fz * my + fx * mx);
    float wl = cadv_sqrtf(wx * wx + wz * wz);
    int dx;
    int dz;
    int ffx;
    int ffz;
    float gap;
    int fy = ifloor(c->pos.y + 0.01f);
    cadv_climber_facing(c, &dx, &dz);
    front(c, d, dx, dz, &ffx, &ffz, &gap);
    if (cadv_input_pressed(in, CADV_ACT_JUMP)) {
        if (grab && gap < 0.35f && cadv_tiles_solid_at(t, ffx, fy, ffz) && ledge_at(t, ffx, fy + 1, ffz) &&
            !cadv_tiles_solid_at(t, ifloor(c->pos.x), fy + 2, ifloor(c->pos.z))) {
            /* climb a one-tile step */
            start_move(c, CADV_CLIMB_PULLUP, cell_top(ffx, fy + 1, ffz), 0.45f);
            return;
        }
        {
            float h = my > 0.5f ? (run ? d->running_jump : d->standing_jump) : 0.0f;
            float jx = fx;
            float jz = fz;
            /* snap to the grid axis when facing close to it (cos 25 deg) */
            if (cadv_absf(fx) > 0.906f) {
                jx = fx > 0 ? 1.0f : -1.0f;
                jz = 0.0f;
            } else if (cadv_absf(fz) > 0.906f) {
                jx = 0.0f;
                jz = fz > 0 ? 1.0f : -1.0f;
            }
            c->vel = cadv_v3(jx * h, d->jump_vy, jz * h);
            c->state = CADV_CLIMB_AIR;
            c->peak_y = c->pos.y;
            *ev |= CADV_CLIMB_EV_JUMP;
            return;
        }
    }
    /* push and pull blocks */
    if (grab && gap < 0.3f && cadv_tiles_get(t, ffx, fy, ffz) == CADV_TILE_BLOCK && cadv_absf(my) > 0.5f) {
        int cx = ifloor(c->pos.x);
        int cz = ifloor(c->pos.z);
        float r = d->radius + 0.02f;
        c->dirx = dx;
        c->dirz = dz;
        if (my > 0.5f) {
            int bx = ffx + dx;
            int bz = ffz + dz;
            if (cadv_tiles_get(t, bx, fy, bz) == CADV_TILE_AIR && cadv_tiles_solid_at(t, bx, fy - 1, bz)) {
                /* the block slides one tile on; the player follows to its new near face */
                float face_x = dx > 0 ? (float)bx : (float)(bx + 1);
                float face_z = dz > 0 ? (float)bz : (float)(bz + 1);
                cadv_tiles_set(t, bx, fy, bz, CADV_TILE_BLOCK);
                cadv_tiles_set(t, ffx, fy, ffz, CADV_TILE_AIR);
                c->pulling = false;
                start_move(c, CADV_CLIMB_PUSH,
                           cadv_v3(dx ? face_x - (float)dx * r : (float)cx + 0.5f, (float)fy, dz ? face_z - (float)dz * r : (float)cz + 0.5f), 0.7f);
                *ev |= CADV_CLIMB_EV_PUSHED;
                return;
            }
        } else {
            int bx = cx - dx;
            int bz = cz - dz;
            if (!cadv_tiles_solid_at(t, bx, fy, bz) && !cadv_tiles_solid_at(t, bx, fy + 1, bz) &&
                cadv_tiles_solid_at(t, bx, fy - 1, bz) && cadv_tiles_get(t, cx, fy, cz) == CADV_TILE_AIR) {
                /* the block follows into the player's tile, the player steps back */
                float face_x = dx > 0 ? (float)cx : (float)(cx + 1);
                float face_z = dz > 0 ? (float)cz : (float)(cz + 1);
                c->pulling = true;
                c->block_x = ffx;
                c->block_y = fy;
                c->block_z = ffz;
                c->block_tx = cx;
                c->block_tz = cz;
                start_move(c, CADV_CLIMB_PUSH,
                           cadv_v3(dx ? face_x - (float)dx * r : (float)cx + 0.5f, (float)fy, dz ? face_z - (float)dz * r : (float)cz + 0.5f), 0.7f);
                return;
            }
        }
    }
    if (wl > 1.0f) {
        wx /= wl;
        wz /= wl;
    }
    slide(c, d, t, wx * speed * dt, wz * speed * dt);
    if (!supported(t, c->pos)) {
        c->state = CADV_CLIMB_AIR;
        c->vel = cadv_v3(wx * speed, 0, wz * speed);
        c->peak_y = c->pos.y;
    }
}

static void tick_air(cadv_climber *c, const cadv_climber_def *d, const cadv_input_frame *in, cadv_tiles *t, float dt, uint32_t *ev)
{
    float ny;
    c->vel.y -= d->gravity * dt;
    if (!slide(c, d, t, c->vel.x * dt, c->vel.z * dt)) {
        /* hit a wall: lose horizontal speed on the blocked axis */
        if (!box_free(t, c->pos.x + c->vel.x * dt, c->pos.y, c->pos.z, d->radius, d->height, 0.05f)) {
            c->vel.x = 0.0f;
        }
        if (!box_free(t, c->pos.x, c->pos.y, c->pos.z + c->vel.z * dt, d->radius, d->height, 0.05f)) {
            c->vel.z = 0.0f;
        }
    }
    ny = c->pos.y + c->vel.y * dt;
    if (c->vel.y < 0.0f && !box_free(t, c->pos.x, ny, c->pos.z, d->radius, d->height, 0.05f)) {
        /* sliding down past an edge: nudge the body out of the face it overlaps
         * (support is tested under the centre, so the body may hang over) */
        float r = d->radius + 0.002f;
        float cand[4][2];
        int k;
        int best = -1;
        float bd = 1e9f;
        cand[0][0] = (float)ifloor(c->pos.x + d->radius) - r;
        cand[0][1] = c->pos.z;
        cand[1][0] = (float)(ifloor(c->pos.x - d->radius) + 1) + r;
        cand[1][1] = c->pos.z;
        cand[2][0] = c->pos.x;
        cand[2][1] = (float)ifloor(c->pos.z + d->radius) - r;
        cand[3][0] = c->pos.x;
        cand[3][1] = (float)(ifloor(c->pos.z - d->radius) + 1) + r;
        for (k = 0; k < 4; ++k) {
            float dd = cadv_absf(cand[k][0] - c->pos.x) + cadv_absf(cand[k][1] - c->pos.z);
            if (dd < bd && dd < 0.6f && box_free(t, cand[k][0], ny, cand[k][1], d->radius, d->height, 0.05f)) {
                bd = dd;
                best = k;
            }
        }
        if (best >= 0) {
            c->pos.x = cand[best][0];
            c->pos.z = cand[best][1];
        }
    }
    if (c->vel.y > 0.0f && !box_free(t, c->pos.x, ny, c->pos.z, d->radius, d->height, 0.05f)) {
        c->vel.y = 0.0f; /* head bump */
        ny = c->pos.y;
    }
    if (c->vel.y <= 0.0f && cadv_tiles_solid_at(t, ifloor(c->pos.x), ifloor(ny), ifloor(c->pos.z)) &&
        (float)(ifloor(ny) + 1) >= ny && (float)(ifloor(ny) + 1) <= c->pos.y + 0.001f) {
        c->pos.y = (float)(ifloor(ny) + 1);
        c->last_fall = c->peak_y - c->pos.y;
        c->state = CADV_CLIMB_GROUND;
        c->vel = cadv_v3(0, 0, 0);
        *ev |= CADV_CLIMB_EV_LAND;
        return;
    }
    c->pos.y = ny;
    if (c->pos.y > c->peak_y) {
        c->peak_y = c->pos.y;
    }
    if (in_water(t, c->pos, 0.9f)) {
        c->state = CADV_CLIMB_SWIM;
        c->vel = cadv_v3_scale(c->vel, 0.25f);
        *ev |= CADV_CLIMB_EV_SPLASH;
        return;
    }
    if (cadv_input_down(in, CADV_ACT_USE) && try_grab(c, d, t)) {
        *ev |= CADV_CLIMB_EV_GRAB;
        return;
    }
    if (c->pos.y < -10.0f) {
        *ev |= CADV_CLIMB_EV_ABYSS;
    }
}

static void tick_hang(cadv_climber *c, const cadv_climber_def *d, const cadv_input_frame *in, cadv_tiles *t, float dt)
{
    int rx = -c->dirz;
    int rz = c->dirx;
    float mx = cadv_clampf(in->move_x, -1, 1);
    if (!cadv_input_down(in, CADV_ACT_USE) || cadv_input_pressed(in, CADV_ACT_CROUCH)) {
        c->state = CADV_CLIMB_AIR;
        c->vel = cadv_v3(0, 0, 0);
        c->peak_y = c->pos.y;
        return;
    }
    if (cadv_input_pressed(in, CADV_ACT_JUMP) || in->move_y > 0.5f) {
        int fx;
        int fz;
        float gap;
        front(c, d, c->dirx, c->dirz, &fx, &fz, &gap);
        start_move(c, CADV_CLIMB_PULLUP, cell_top(fx, c->ledge_y, fz), 0.6f);
        return;
    }
    if (mx != 0.0f) {
        float sx = (float)rx * mx * 1.2f * dt;
        float sz = (float)rz * mx * 1.2f * dt;
        cadv_climber probe = *c;
        int fx;
        int fz;
        float gap;
        probe.pos.x += sx + (float)rx * (mx > 0 ? d->radius : -d->radius);
        probe.pos.z += sz + (float)rz * (mx > 0 ? d->radius : -d->radius);
        front(&probe, d, c->dirx, c->dirz, &fx, &fz, &gap);
        if (ledge_at(t, fx, c->ledge_y, fz) && box_free(t, c->pos.x + sx, c->pos.y, c->pos.z + sz, d->radius, d->height, 0.0f)) {
            c->pos.x += sx;
            c->pos.z += sz;
        }
    }
}

static void tick_swim(cadv_climber *c, const cadv_climber_def *d, const cadv_input_frame *in, cadv_tiles *t, float dt, uint32_t *ev)
{
    cadv_vec3 f = cadv_climber_forward(c);
    float rx = cadv_cosf(c->yaw);
    float rz = -cadv_sinf(c->yaw);
    float mx = cadv_clampf(in->move_x, -1, 1);
    float my = cadv_clampf(in->move_y, -1, 1);
    float vy = f.y * my * d->swim;
    float ny;
    bool surface = !in_water(t, c->pos, 1.2f);
    if (cadv_input_down(in, CADV_ACT_JUMP)) {
        vy += 2.0f;
    }
    if (cadv_input_down(in, CADV_ACT_CROUCH)) {
        vy -= 2.0f;
    }
    if (vy == 0.0f) {
        vy = surface ? 0.0f : -0.2f;
    }
    /* climb out at the surface onto a low edge */
    if (surface && cadv_input_pressed(in, CADV_ACT_JUMP)) {
        int dx;
        int dz;
        int fx;
        int fz;
        float gap;
        int s = ifloor(c->pos.y + 1.0f);
        cadv_climber_facing(c, &dx, &dz);
        front(c, d, dx, dz, &fx, &fz, &gap);
        if (gap < 0.4f) {
            int k;
            for (k = 0; k <= 1; ++k) {
                if (ledge_at(t, fx, s + k, fz)) {
                    start_move(c, CADV_CLIMB_PULLUP, cell_top(fx, s + k, fz), 0.6f);
                    return;
                }
            }
        }
    }
    slide(c, d, t, (f.x * my + rx * mx) * d->swim * dt, (f.z * my + rz * mx) * d->swim * dt);
    ny = c->pos.y + vy * dt;
    if (vy > 0.0f && surface) {
        ny = c->pos.y; /* float at the surface, head out */
    }
    if (box_free(t, c->pos.x, ny, c->pos.z, d->radius, d->height, 0.0f)) {
        c->pos.y = ny;
    }
    if (in_water(t, c->pos, d->eye)) {
        c->air -= dt;
        if (c->air <= 0.0f) {
            c->air = 0.0f;
            *ev |= CADV_CLIMB_EV_DROWNING;
        }
    }
    if (!in_water(t, c->pos, 0.9f)) {
        c->state = supported(t, c->pos) ? CADV_CLIMB_GROUND : CADV_CLIMB_AIR;
        c->vel = cadv_v3(0, 0, 0);
        c->peak_y = c->pos.y;
    }
}

uint32_t cadv_climber_tick(cadv_climber *c, const cadv_climber_def *d, const cadv_input_frame *in, cadv_tiles *t, float dt)
{
    uint32_t ev = 0;
    c->yaw = cadv_wrap_angle(c->yaw + in->look_x);
    c->pitch = cadv_clampf(c->pitch + in->look_y, -1.4f, 1.4f);
    if (!in_water(t, c->pos, d->eye)) {
        c->air = cadv_minf(d->air_max, c->air + 8.0f * dt);
    }
    switch (c->state) {
    case CADV_CLIMB_GROUND:
        tick_ground(c, d, in, t, dt, &ev);
        break;
    case CADV_CLIMB_AIR:
        tick_air(c, d, in, t, dt, &ev);
        break;
    case CADV_CLIMB_HANG:
        tick_hang(c, d, in, t, dt);
        break;
    case CADV_CLIMB_SWIM:
        tick_swim(c, d, in, t, dt, &ev);
        break;
    default: /* pull-up and push follow a timed path */
        c->timer += dt;
        c->pos = cadv_v3_lerp(c->from, c->to, cadv_minf(1.0f, c->timer / c->duration));
        if (c->timer >= c->duration) {
            if (c->state == CADV_CLIMB_PUSH && c->pulling) {
                cadv_tiles_set(t, c->block_tx, c->block_y, c->block_tz, CADV_TILE_BLOCK);
                cadv_tiles_set(t, c->block_x, c->block_y, c->block_z, CADV_TILE_AIR);
                ev |= CADV_CLIMB_EV_PUSHED;
            } else if (c->state == CADV_CLIMB_PULLUP) {
                ev |= CADV_CLIMB_EV_UP;
            }
            c->state = in_water(t, c->pos, 0.9f) ? CADV_CLIMB_SWIM : CADV_CLIMB_GROUND;
            c->peak_y = c->pos.y;
        }
        break;
    }
    return ev;
}
