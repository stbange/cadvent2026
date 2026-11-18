/* SPDX-License-Identifier: MIT */
/* ppmprobe: asserts pixel colours in a PPM screenshot (used by game tests).
 *   ppmprobe FILE X% Y% R G B [TOLERANCE]
 * X% and Y% are positions in percent of width/height (0..100). Exit code 0
 * when the pixel matches within TOLERANCE per channel (default 2). */
#include <stdio.h>
#include <stdlib.h>

#include "core/image.h"

int main(int argc, char **argv)
{
    cadv_image img;
    int v[6];
    int tol = 2;
    int i;
    int x;
    int y;
    uint8_t px[3];
    if (argc != 7 && argc != 8) {
        fprintf(stderr, "usage: ppmprobe FILE X%% Y%% R G B [TOLERANCE]\n");
        return 2;
    }
    for (i = 0; i < 5; ++i) {
        v[i] = atoi(argv[i + 2]);
    }
    if (argc == 8) {
        tol = atoi(argv[7]);
    }
    if (!cadv_image_read_ppm(&img, argv[1])) {
        fprintf(stderr, "ppmprobe: cannot read %s\n", argv[1]);
        return 2;
    }
    x = (img.width - 1) * v[0] / 100;
    y = (img.height - 1) * v[1] / 100;
    cadv_image_get(&img, x, y, px);
    cadv_image_free(&img);
    for (i = 0; i < 3; ++i) {
        if (abs((int)px[i] - v[2 + i]) > tol) {
            fprintf(stderr, "ppmprobe: pixel (%d,%d) is %d,%d,%d, expected %d,%d,%d\n", x, y,
                    px[0], px[1], px[2], v[2], v[3], v[4]);
            return 1;
        }
    }
    printf("ppmprobe: pixel (%d,%d) = %d,%d,%d ok\n", x, y, px[0], px[1], px[2]);
    return 0;
}
