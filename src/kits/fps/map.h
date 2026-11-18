/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_FPS_MAP_H
#define CADV_KITS_FPS_MAP_H

#include <stdbool.h>
#include <stdint.h>

#include "core/mathx.h"
#include "kits/fps/combat.h"

/* Text level format for FPS-style portals (assets/maps/NAME.map, CC BY 4.0).
 * One entity per line, '#' starts a comment, numbers in metres/radians:
 *
 *   name TEXT...
 *   spawn x y z yaw
 *   box    x0 y0 z0 x1 y1 z1 MAT          solid, drawn
 *   deco   x0 y0 z0 x1 y1 z1 MAT          drawn only
 *   glass  x0 y0 z0 x1 y1 z1 HP MAT       breakable (group 0)
 *   target x0 y0 z0 x1 y1 z1 HP           breakable objective (group 1)
 *   door   x0 y0 z0 x1 y1 z1 KEYBITS MAT
 *   enemy  TYPE x y z yaw
 *   pickup KIND VALUE SUB x y z           health armor ammo key weapon star jetpack fuel
 *   prop   KIND x y z yaw                 portal-defined usable object (adds a solid)
 *   secret x0 y0 z0 x1 y1 z1
 *   trigger NAME x0 y0 z0 x1 y1 z1
 *   light  x y z r g b radius SHADOW
 *   sign   x y z yaw width TEXT...
 *   mark   INDEX x y z yaw                friend mark plaque (portal mark index)
 *   waypoint x y z [ACTION]               ordered path for the completion bot
 */

#define CADV_MAP_MAX_BOXES 512
#define CADV_MAP_MAX_ITEMS 128
#define CADV_MAP_NAME 32
#define CADV_MAP_TEXT 48

typedef struct cadv_map_box {
    cadv_vec3 lo, hi;
    char mat[CADV_MAP_NAME];
    int kind;   /* 0 solid, 1 deco, 2 glass, 3 target, 4 door */
    int value;  /* HP or key bits */
} cadv_map_box;

typedef struct cadv_map_item {
    char name[CADV_MAP_NAME]; /* enemy type, pickup kind, prop kind, trigger name, action */
    cadv_vec3 pos;            /* or box lo */
    cadv_vec3 hi;             /* box hi for volumes / colour for lights */
    float yaw;
    float a, b;               /* value/sub, radius/shadow, width ... */
    char text[CADV_MAP_TEXT];
} cadv_map_item;

typedef struct cadv_map {
    char name[CADV_MAP_TEXT];
    cadv_vec3 spawn;
    float spawn_yaw;
    cadv_map_box box[CADV_MAP_MAX_BOXES];
    int nbox;
    cadv_map_item enemy[CADV_MAP_MAX_ITEMS];
    int nenemy;
    cadv_map_item pickup[CADV_MAP_MAX_ITEMS];
    int npickup;
    cadv_map_item prop[CADV_MAP_MAX_ITEMS];
    int nprop;
    cadv_map_item secret[16];
    int nsecret;
    cadv_map_item trigger[32];
    int ntrigger;
    cadv_map_item light[CADV_MAP_MAX_ITEMS];
    int nlight;
    cadv_map_item sign[32];
    int nsign;
    cadv_map_item mark[16];
    int nmark;
    cadv_map_item waypoint[CADV_MAP_MAX_ITEMS];
    int nwaypoint;
    char error[96];
} cadv_map;

/* Parses map text; returns false with error set ("line N: ..."). */
bool cadv_map_parse(cadv_map *m, const char *text);
bool cadv_map_load(cadv_map *m, const char *path);
int cadv_map_pickup_kind(const char *name); /* -1 if unknown */
const cadv_map_item *cadv_map_find_trigger(const cadv_map *m, const char *name);

/* Indices of the map objects inside the FPS world after applying. */
typedef struct cadv_map_links {
    int box_solid[CADV_MAP_MAX_BOXES];   /* world solid, door index or breakable index; -1 for deco */
    int prop_solid[CADV_MAP_MAX_ITEMS];  /* world solid of each prop */
    int enemy_index[CADV_MAP_MAX_ITEMS];
    int pickup_index[CADV_MAP_MAX_ITEMS];
} cadv_map_links;

/* Fills an FPS world from the map: solids, doors, breakables, prop solids,
 * enemies (types resolved by 'enemy_type'), pickups and the player spawn.
 * Enemy names not known to 'enemy_type' (returns -1) are skipped. */
void cadv_map_apply(const cadv_map *m, cadv_fps *f, int (*enemy_type)(const char *name),
                    cadv_map_links *links);

#endif
