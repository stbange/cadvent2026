/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_FONT_H
#define CADV_GFX_FONT_H

#include <stdbool.h>
#include <stdint.h>

/* Original 5x7 pixel font (glyph art CC BY 4.0) covering printable ASCII.
 * Each glyph cell is 6x9 pixels including spacing. */
#define CADV_FONT_GLYPH_W 5
#define CADV_FONT_GLYPH_H 7
#define CADV_FONT_ADVANCE 6
#define CADV_FONT_LINE 9
/* Atlas: 16 x 6 cells of 8x8 pixels (glyphs 32..127; 127 = solid block). */
#define CADV_FONT_ATLAS_W 128
#define CADV_FONT_ATLAS_H 48

/* Returns the 7 row bitmasks (bit 4 = leftmost pixel); unknown chars map to '?'. */
const uint8_t *cadv_font_glyph(int ch);
bool cadv_font_pixel(int ch, int x, int y);
/* RGBA atlas, white glyphs on transparent; 'rgba' must hold W*H*4 bytes. */
void cadv_font_build_atlas(uint8_t *rgba);
/* Atlas cell origin (pixels) for a character. */
void cadv_font_cell(int ch, int *x, int *y);
/* Width and height in pixels of (possibly multi-line) text at scale 1. */
void cadv_font_measure(const char *text, int *w, int *h);

#endif
