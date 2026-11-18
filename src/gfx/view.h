/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_VIEW_H
#define CADV_GFX_VIEW_H

#include <stdbool.h>

#include "core/mathx.h"

/* Pure helpers for cameras, split screen, sprites and light selection. */

typedef struct cadv_rect { int x, y, w, h; } cadv_rect;

/* Splits a window of w x h pixels into n (1..4) player views: 1 = full,
 * 2 = side by side, 3 = two on top + one wide bottom, 4 = quad. Rect y is
 * measured from the bottom (OpenGL viewport convention). */
cadv_rect cadv_view_split(int n, int index, int w, int h);
/* Low-res render target size for a view: height fixed, width follows aspect
 * (rounded to an even number, at least 2). */
void cadv_view_internal_size(int view_w, int view_h, int internal_h, int *out_w, int *out_h);

typedef struct cadv_camera {
    cadv_vec3 pos;
    float yaw, pitch;
    float fovy; /* radians */
    float znear, zfar;
} cadv_camera;

cadv_mat4 cadv_camera_view(const cadv_camera *c);
cadv_mat4 cadv_camera_proj(const cadv_camera *c, float aspect);

/* Which of 'dirs' sprite frames to show: 0 = sprite faces the viewer.
 * Frames go counter-clockwise (frame k shows the sprite rotated by k*360/dirs). */
int cadv_sprite_frame(cadv_vec3 sprite_pos, float sprite_yaw, cadv_vec3 viewer_pos, int dirs);

typedef struct cadv_light_ref {
    cadv_vec3 pos;
    float radius;
    float intensity;
    bool casts_shadow;
} cadv_light_ref;

/* Picks up to 'max_out' shadow-casting lights that matter most for a camera
 * at 'eye' (closest first, weighted by intensity and radius). Writes their
 * indices into 'out' and returns the count. */
int cadv_select_shadow_lights(const cadv_light_ref *lights, int n, cadv_vec3 eye, int *out,
                              int max_out);

#endif
