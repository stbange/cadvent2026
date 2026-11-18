/* SPDX-License-Identifier: MIT */
#ifndef CADV_HUB_LAYOUT_H
#define CADV_HUB_LAYOUT_H

#include "core/mathx.h"
#include "sim/collide.h"

/* Geometry of the Hall of Long Nights (metres, Y up, north = -Z):
 *   main hall x -16..16, z -32..8, height 12, entrance on the south side
 *   apse (Day 24, Star Arch) x -6..6, z -40..-32
 *   balconies at y = 4 along both side walls, reached by stairs in the south
 *   days 1-6 west ground wall, 7-12 east ground wall,
 *   13-18 west balcony, 19-23 east balcony, 24 in the apse. */

#define CADV_HALL_W 16.0f        /* half width */
#define CADV_HALL_SOUTH 8.0f
#define CADV_HALL_NORTH -32.0f
#define CADV_HALL_HEIGHT 12.0f
#define CADV_APSE_NORTH -40.0f
#define CADV_APSE_HALF 6.0f
#define CADV_BALCONY_Y 4.0f
#define CADV_BALCONY_DEPTH 4.0f
#define CADV_ARCH_WIDTH 2.4f
#define CADV_ARCH_HEIGHT 3.4f

typedef struct cadv_portal_slot {
    cadv_vec3 base;   /* bottom centre of the arch opening, on the wall surface */
    float yaw;        /* direction the portal faces (into the hall) */
    cadv_vec3 normal; /* unit vector pointing into the hall */
} cadv_portal_slot;

cadv_portal_slot cadv_hall_portal(int day);
/* Point in front of a portal where the player stands to look at it. */
cadv_vec3 cadv_hall_portal_front(int day, float distance);
/* Star Arch socket position for days 1..23 (arc around portal 24). */
cadv_vec3 cadv_hall_socket(int day);
cadv_vec3 cadv_hall_tree(void);
cadv_vec3 cadv_hall_spawn(void);
float cadv_hall_spawn_yaw(void);
cadv_vec3 cadv_hall_legend_plaque(int index); /* Wall of Legends, 6 plaques */
cadv_vec3 cadv_hall_arch_use_point(void);     /* where USE sockets the stars */
/* Adds all walls, floors, stairs, railings, columns and the tree to the world. */
void cadv_hall_build_world(cadv_world *w);
/* Waypoints (feet positions) from the spawn to stand in front of portal 'day'.
 * Returns the count written (<= max). */
int cadv_hall_path(int day, cadv_vec3 *out, int max);

#endif
