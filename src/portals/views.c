/* SPDX-License-Identifier: MIT */
/* GPU hooks of the 24 portals (see cadv_portal_view). */
#include "portal/portal.h"
#include "portals/test_range_gfx.h"

static const cadv_portal_view k_test_range_view = {cadv_test_range_gpu_init, cadv_test_range_gpu_free,
                                                   cadv_test_range_draw};

const cadv_portal_view *cadv_portal_view_get(int day)
{
    if (day < 1 || day > CADV_PORTAL_COUNT) {
        return NULL;
    }
    return &k_test_range_view;
}
