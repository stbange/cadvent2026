/* SPDX-License-Identifier: MIT */
/* imgdiff: golden-frame comparison for game tests.
 *   imgdiff ACTUAL.ppm EXPECTED.ppm [CHANNEL_TOL] [MAX_PERCENT] [DIFF.ppm]
 * A pixel differs when any channel differs by more than CHANNEL_TOL (default
 * 24). Exit 0 if at most MAX_PERCENT (default 2.0) of the pixels differ. */
#include <stdio.h>
#include <stdlib.h>

#include "core/image.h"

int main(int argc, char **argv)
{
    cadv_image a;
    cadv_image b;
    cadv_image d;
    int tol = 24;
    double max_pct = 2.0;
    long diff = 0;
    long n;
    long i;
    double pct;
    if (argc < 3 || argc > 6) {
        fprintf(stderr, "usage: imgdiff ACTUAL EXPECTED [CHANNEL_TOL] [MAX_PERCENT] [DIFF.ppm]\n");
        return 2;
    }
    if (argc >= 4) {
        tol = atoi(argv[3]);
    }
    if (argc >= 5) {
        max_pct = atof(argv[4]);
    }
    if (!cadv_image_read_ppm(&a, argv[1]) || !cadv_image_read_ppm(&b, argv[2])) {
        fprintf(stderr, "imgdiff: cannot read %s or %s\n", argv[1], argv[2]);
        return 2;
    }
    if (a.width != b.width || a.height != b.height) {
        fprintf(stderr, "imgdiff: size mismatch %dx%d vs %dx%d\n", a.width, a.height, b.width, b.height);
        cadv_image_free(&a);
        cadv_image_free(&b);
        return 1;
    }
    cadv_image_alloc(&d, a.width, a.height);
    n = (long)a.width * a.height;
    for (i = 0; i < n; ++i) {
        int c;
        int worst = 0;
        for (c = 0; c < 3; ++c) {
            int x = abs((int)a.rgb[i * 3 + c] - (int)b.rgb[i * 3 + c]);
            worst = x > worst ? x : worst;
        }
        if (worst > tol) {
            ++diff;
            d.rgb[i * 3] = 255;
        } else {
            d.rgb[i * 3 + 1] = (unsigned char)(a.rgb[i * 3 + 1] / 3);
        }
    }
    pct = 100.0 * (double)diff / (double)n;
    if (argc == 6) {
        cadv_image_write_ppm(&d, argv[5]);
    }
    cadv_image_free(&a);
    cadv_image_free(&b);
    cadv_image_free(&d);
    printf("imgdiff: %.3f%% of pixels differ (limit %.3f%%)\n", pct, max_pct);
    return pct <= max_pct ? 0 : 1;
}
