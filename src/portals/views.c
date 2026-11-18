/* SPDX-License-Identifier: MIT */
/* GPU hooks of the 24 portals (see cadv_portal_view). */
#include "portal/portal.h"
#include "portals/p01_sleighed_gfx.h"
#include "portals/p02_aurora_carpet_gfx.h"
#include "portals/test_range_gfx.h"

static const cadv_portal_view k_test_range_view = {cadv_test_range_gpu_init, cadv_test_range_gpu_free,
                                                   cadv_test_range_draw};

const cadv_portal_view *cadv_portal_view_get(int day)
{
    static const cadv_portal_view k_p01 = {cadv_p01_gpu_init, cadv_p01_gpu_free, cadv_p01_draw};
    static const cadv_portal_view k_p02 = {cadv_p02_gpu_init, cadv_p02_gpu_free, cadv_p02_draw};
    switch (day) {
    case 1:
        return &k_p01;
    case 2:
        return &k_p02;
    default:
        return day < 1 || day > CADV_PORTAL_COUNT ? NULL : &k_test_range_view;
    }
}
