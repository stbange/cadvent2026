/* SPDX-License-Identifier: MIT */
/* The 24 portals in calendar order. Until a portal's real game ships it is
 * backed by the shared test range and marked not implemented ("still
 * thawing"): it can then only be entered with the development override. */
#include "portal/portal.h"
#include "portals/p01_sleighed.h"
#include "portals/p02_aurora_carpet.h"
#include "portals/test_range.h"

static const cadv_mark_def k_marks_01[] = {
    {0, "HIGH SCORE: %s"},
    {1, "%s WAS HERE"},
    {4, "BEWARE OF %s"},
    {5, "%s HAIR STUDIO"},
};
static const cadv_mark_def k_marks_02[] = {
    {0, "EVEN %s COULD NOT HOLD THIS MUCH MANA"},
    {2, "%s"},
    {4, "%s'S ROCK"},
};
static const cadv_mark_def k_marks_03[] = {
    {0, "%s RESEARCH LAB"},
    {1, "%s SQUAD, HOLD THE BRIDGE"},
    {3, "CALLSIGN %s"},
};
static const cadv_mark_def k_marks_04[] = {
    {2, "%s - DAY 41"},
    {4, "BEWARE OF %s"},
    {5, "JOURNAL OF %s"},
};
static const cadv_mark_def k_marks_05[] = {
    {1, "%s THE UNWASHED"},
    {3, "SCOUT %s"},
    {4, "%s SHARPENED MY AXE"},
    {5, "%s: WARM MEAD, HOT AIR"},
};
static const cadv_mark_def k_marks_06[] = {
    {0, "PAINT: %s BLACK"},
    {1, "%s WAS HERE"},
    {2, "WINGMAN %s"},
    {5, "WINGMAN %s"},
};
static const cadv_mark_def k_marks_07[] = {
    {0, "%s DID THIS"},
    {1, "BOT %s"},
    {2, "BOT %s"},
    {3, "BOT %s"},
    {4, "BOT %s"},
    {5, "BOT %s"},
};
static const cadv_mark_def k_marks_08[] = {
    {0, "%s - RESONANCE LAB - DO NOT ENTER"},
    {2, "MAINTENANCE BOT %s"},
    {3, "RADIOLOGY: %s"},
};
static const cadv_mark_def k_marks_09[] = {
    {1, "%s SPECIAL"},
    {2, "BOT %s"},
    {3, "BOT %s"},
    {5, "%s HALFPIPE"},
};
static const cadv_mark_def k_marks_10[] = {
    {2, "%s CYBERNETICS"},
    {3, "AGENT %s"},
    {4, "%s WAS HERE"},
};
static const cadv_mark_def k_marks_11[] = {
    {1, "%s SHAFT"},
    {2, "MINER %s"},
    {3, "MINER %s"},
    {4, "BEWARE OF %s"},
};
static const cadv_mark_def k_marks_12[] = {
    {1, "HERE LIES %s, HE MADE TOO MUCH"},
    {4, "%s'S WHITTLING KNIFE"},
    {5, "HEALER %s"},
};
static const cadv_mark_def k_marks_13[] = {
    {0, "%s DID THIS"},
    {1, "PAR TIME SET BY %s"},
    {2, "DOG TAGS: %s"},
};
static const cadv_mark_def k_marks_14[] = {
    {0, "%s THE RABBIT-BLOB"},
    {4, "KNIFE-GRINDER %s"},
    {5, "INNKEEPER %s"},
};
static const cadv_mark_def k_marks_15[] = {
    {0, "PVT. %s"},
    {1, "SGT. %s"},
    {2, "PVT. %s"},
    {3, "CPL. %s"},
    {4, "PVT. %s"},
    {5, "PVT. %s"},
};
static const cadv_mark_def k_marks_16[] = {
    {0, "%s'S RUNE LAB"},
    {1, "%s WAS HERE"},
    {2, "BOT %s"},
    {3, "BOT %s"},
    {4, "BOT %s"},
    {5, "BOT %s"},
};
static const cadv_mark_def k_marks_17[] = {
    {2, "TRAINING REMOTE %s"},
    {4, "%s THE BLADE-SAINT"},
    {5, "%s OF THE WARM WINDS"},
};
static const cadv_mark_def k_marks_18[] = {
    {0, "GENERAL %s"},
    {4, "FORT %s"},
    {5, "CAMP %s"},
};
static const cadv_mark_def k_marks_19[] = {
    {2, "%s"},
    {3, "LOG OF %s"},
    {4, "BEWARE OF %s"},
};
static const cadv_mark_def k_marks_20[] = {
    {0, "AUDIO LOG: %s"},
    {1, "AUDIO LOG: %s"},
    {2, "ANOTHER %s-MODEL MISTAKE"},
    {5, "%s: HAIR DRYER, DO NOT BORROW"},
};
static const cadv_mark_def k_marks_21[] = {
    {1, "CHIEFTAIN %s"},
    {3, "EYE OF %s"},
    {4, "KNIFE OF %s"},
    {5, "BREATH OF %s"},
};
static const cadv_mark_def k_marks_22[] = {
    {0, "HERE LIES %s"},
    {1, "HERE LIES %s"},
    {4, "BEWARE OF %s"},
    {5, "HIGH SCORE: %s"},
};
static const cadv_mark_def k_marks_23[] = {
    {0, "%s DID THIS"},
    {2, "WRECK: %s"},
    {3, "WRECK: %s"},
};
static const cadv_mark_def k_marks_24[] = {
    {0, "GENERAL %s"},
    {1, "GENERAL %s"},
    {2, "GENERAL %s"},
    {3, "GENERAL %s"},
    {4, "GENERAL %s"},
    {5, "GENERAL %s"},
};

#define MARKS(d) k_marks_##d, (int)(sizeof k_marks_##d / sizeof k_marks_##d[0])

static const cadv_portal_desc k_portals[CADV_PORTAL_COUNT] = {
    {1, "p01_sleighed", "Sleighed!", "Duke Nukem 3D (1996)", CADV_CAP_SOLO | CADV_CAP_COOP | CADV_CAP_VERSUS | CADV_CAP_XR_SEATED,
     true, CADV_P01_STATE_SIZE, cadv_p01_init, cadv_p01_tick, cadv_p01_result, cadv_p01_query, cadv_p01_bot,
     cadv_p01_teleport, MARKS(01)},
    {2, "p02_aurora_carpet", "Aurora Carpet", "Magic Carpet 2 (1995)", CADV_CAP_SOLO | CADV_CAP_VERSUS | CADV_CAP_XR_SEATED,
     true, CADV_P02_STATE_SIZE, cadv_p02_init, cadv_p02_tick, cadv_p02_result, cadv_p02_query, cadv_p02_bot,
     cadv_p02_teleport, MARKS(02)},
    CADV_TEST_RANGE_DESC(3, "p03_frostfront", "Frostfront", "Command & Conquer (1995)", CADV_CAP_SOLO|CADV_CAP_VERSUS|CADV_CAP_XR_TABLETOP, MARKS(03)),
    CADV_TEST_RANGE_DESC(4, "p04_ice_tomb", "Ice Tomb of the Winter King", "Tomb Raider (1996)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_XR_SEATED, MARKS(04)),
    CADV_TEST_RANGE_DESC(5, "p05_hearthguard", "Hearthguard", "Warcraft II (1995)", CADV_CAP_SOLO|CADV_CAP_VERSUS|CADV_CAP_XR_TABLETOP, MARKS(05)),
    CADV_TEST_RANGE_DESC(6, "p06_iron_yule", "Iron Yule", "MechWarrior 2 (1995)", CADV_CAP_SOLO|CADV_CAP_VERSUS|CADV_CAP_XR_FULL, MARKS(06)),
    CADV_TEST_RANGE_DESC(7, "p07_tinsel_strike", "Tinsel Strike", "Counter-Strike (1999)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_VERSUS|CADV_CAP_XR_FULL, MARKS(07)),
    CADV_TEST_RANGE_DESC(8, "p08_cascade", "Cascade", "Half-Life (1998)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_XR_FULL, MARKS(08)),
    CADV_TEST_RANGE_DESC(9, "p09_skifrost", "Skifrost", "Starsiege: Tribes (1998)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_VERSUS|CADV_CAP_XR_SEATED, MARKS(09)),
    CADV_TEST_RANGE_DESC(10, "p10_neon_syndic", "Neon Syndic", "Syndicate Wars (1996)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_XR_SEATED, MARKS(10)),
    CADV_TEST_RANGE_DESC(11, "p11_deep_thaw", "Deep Thaw", "Descent (1995)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_VERSUS|CADV_CAP_XR_FULL, MARKS(11)),
    CADV_TEST_RANGE_DESC(12, "p12_below_the_chapel", "Below the Chapel", "Diablo (1996)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_XR_SEATED, MARKS(12)),
    CADV_TEST_RANGE_DESC(13, "p13_hellfrost_station", "Hellfrost Station", "Doom (1993)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_VERSUS|CADV_CAP_XR_FULL, MARKS(13)),
    CADV_TEST_RANGE_DESC(14, "p14_ring_of_the_evergreen", "Ring of the Evergreen", "Secret of Mana (1993)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_XR_SEATED, MARKS(14)),
    CADV_TEST_RANGE_DESC(15, "p15_frostfall_protocol", "Frostfall Protocol", "X-COM: UFO Defense (1994)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_XR_TABLETOP, MARKS(15)),
    CADV_TEST_RANGE_DESC(16, "p16_rune_of_rime", "Rune of Rime", "Quake (1996)", CADV_CAP_SOLO|CADV_CAP_VERSUS|CADV_CAP_XR_FULL, MARKS(16)),
    CADV_TEST_RANGE_DESC(17, "p17_way_of_the_glowblade", "Way of the Glowblade", "Jedi Knight (1997)", CADV_CAP_SOLO|CADV_CAP_VERSUS|CADV_CAP_XR_FULL, MARKS(17)),
    CADV_TEST_RANGE_DESC(18, "p18_hivemind", "Hivemind", "StarCraft (1998)", CADV_CAP_SOLO|CADV_CAP_VERSUS|CADV_CAP_XR_TABLETOP, MARKS(18)),
    CADV_TEST_RANGE_DESC(19, "p19_skyfall_isle", "Skyfall Isle", "Unreal (1998)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_XR_FULL, MARKS(19)),
    CADV_TEST_RANGE_DESC(20, "p20_ghost_in_the_orbiter", "Ghost in the Orbiter", "System Shock (1994)", CADV_CAP_SOLO|CADV_CAP_XR_SEATED, MARKS(20)),
    CADV_TEST_RANGE_DESC(21, "p21_ages_of_midwinter", "Ages of Midwinter", "Age of Empires (1997)", CADV_CAP_SOLO|CADV_CAP_VERSUS|CADV_CAP_XR_TABLETOP, MARKS(21)),
    CADV_TEST_RANGE_DESC(22, "p22_grave_tidings", "Grave Tidings", "Blood (1997)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_VERSUS|CADV_CAP_XR_SEATED, MARKS(22)),
    CADV_TEST_RANGE_DESC(23, "p23_total_thaw", "Total Thaw", "Total Annihilation (1997)", CADV_CAP_SOLO|CADV_CAP_VERSUS|CADV_CAP_XR_TABLETOP, MARKS(23)),
    CADV_TEST_RANGE_DESC(24, "p24_polar_alert", "Polar Alert", "Command & Conquer: Red Alert (1996)", CADV_CAP_SOLO|CADV_CAP_COOP|CADV_CAP_VERSUS|CADV_CAP_XR_TABLETOP, MARKS(24)),
};

const cadv_portal_desc *cadv_portal_get(int day)
{
    if (day < 1 || day > CADV_PORTAL_COUNT) {
        return NULL;
    }
    return &k_portals[day - 1];
}
