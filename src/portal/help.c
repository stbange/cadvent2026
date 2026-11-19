/* SPDX-License-Identifier: MIT */
#include "portal/help.h"

#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "gfx/font.h"

#define MOVE {"W A S D", "L-STICK", "MOVE"}
#define LOOK {"MOUSE", "R-STICK", "LOOK"}
#define HELP {"F1 / H", "BACK", "THIS HELP (PAUSES)"}
#define LEAVE {"ESC", "START", "BACK TO THE HALL"}

static const cadv_help k_hall = {
    "THE HALL OF LONG NIGHTS",
    "WALK INTO A GLOWING PORTAL TO PLAY ITS GAME. A NEW ONE OPENS EVERY DAY FROM 1 TO 24 DECEMBER. "
    "BRING THE SHINING STAR OF EACH WORLD BACK AND SET IT INTO THE STAR ARCH: ALL 23 OPEN PORTAL 24.",
    {MOVE, LOOK, {"SHIFT", "L3", "RUN"}, {"SPACE", "A", "JUMP"}, {"E", "X", "READ / SET A STAR"}, HELP},
    6,
    {"PORTALS OPEN AT YOUR LOCAL MIDNIGHT.", "SNOWFLAKES RATE YOUR BEST RUN: A WIN AND TWO CHALLENGES.",
     "FRIENDS' MARKS HIDE IN EVERY WORLD - THE WALL OF LEGENDS COUNTS THEM."},
    3,
};

static const cadv_help k_day1 = {
    "DAY 1 - SLEIGHED!",
    "TAKE BACK THE GLITTERDOME MALL. FIND THE THREE FUSE CANDLES, OPEN THE STAFF DOOR AND SMASH THE "
    "LIGHT SIPHON'S THREE EMITTERS ON THE ROOF. GRAB THE SHINING STAR AND REACH THE SLEIGH PAD BEFORE "
    "THE DOME CAVES IN.",
    {MOVE, LOOK, {"L-MOUSE", "RT", "FIRE"}, {"R-MOUSE", "LT", "ALT-FIRE / DETONATE"}, {"SPACE", "A", "JUMP / JETPACK"},
     {"C", "B", "CROUCH"}, {"SHIFT", "L3", "RUN"}, {"E", "X", "USE DOORS & PROPS"}, {"1 - 6", "LB / RB", "WEAPONS"}, HELP, LEAVE},
    11,
    {"SHRUNK ENEMIES CAN BE STOMPED.", "SIX SECRETS HIDE IN THE MALL - TRY THE ARCADE.",
     "GIFT-BOMBS WAIT FOR ALT-FIRE TO GO OFF."},
    3,
};

static const cadv_help k_day2 = {
    "DAY 2 - AURORA CARPET",
    "CLAIM MANA ORBS - YOUR BALLOONS CARRY THEM HOME. HOLD 75% OF THE WORLD'S MANA, OR BRING DOWN "
    "VAEL'S CASTLE AFTER NIGHTFALL, THEN FLY INTO THE SHINING STAR.",
    {{"W A S D", "L-STICK", "FLY / STRAFE"}, LOOK, {"SPACE", "A", "CLIMB"}, {"C", "B", "SINK"},
     {"SHIFT", "L3", "BOOST"}, {"L-MOUSE", "RT", "CAST THE SPELL"}, {"R-MOUSE", "LT", "CLAIM MANA"},
     {"1 - 6", "LB / RB", "CHOOSE A SPELL"}, HELP, LEAVE},
    10,
    {"CAST CASTLE (5) ON FLAT LAND FIRST.", "ONLY FETCHED MANA COUNTS: GUARD YOUR BALLOONS.",
     "RAISE LAND (3) BLOCKS RAIDERS. AT NIGHT A VOLCANO SCROLL GLOWS IN THE WEST CRATER.",
     "VAEL'S CASTLE IS WARDED UNTIL NIGHTFALL."},
    4,
};

static const cadv_help k_day3 = {
    "DAY 3 - FROSTFRONT",
    "BUILD A BASE, TRAIN AN ARMY AND DESTROY THE EMBER SPIRE. THEN SEND AN ENGINEER INTO ITS VAULT TO "
    "CARRY THE SHINING STAR BACK TO YOU.",
    {MOVE, LOOK, {"Q / WHEEL", "LB / RB", "PICK TABLET ITEM"}, {"E", "X", "BUY / PLACE AGAIN"},
     {"L-MOUSE", "RT", "PLACE / PICK / ORDER"}, {"R-MOUSE", "LT", "SELECT NEAR AIM"}, {"SHIFT", "L3", "ADD / PLAIN MOVE"},
     {"C", "B", "CLEAR SQUAD"}, {"1 - 4", "-", "GROUP (SHIFT: SET)"}, {"5 / 6", "-", "ARMY / ENGINEERS"},
     {"SPACE", "A", "RECON DRONE"}, {"TAB", "D-UP", "BIG MAP"}, HELP},
    13,
    {"POWER FIRST, THEN A REFINERY (FREE HARVESTER).", "FROSTITE CRYSTALS HURT INFANTRY.",
     "AN ENGINEER CAN CAPTURE THE RESEARCH LAB.", "NO SQUAD SELECTED? FIRE USES YOUR SIDEARM."},
    4,
};

static const cadv_help k_day4 = {
    "DAY 4 - ICE TOMB",
    "FIND THE WINTER KING'S THREE SUN GEARS IN THE GLACIER TOMB AND SET THEM INTO THE SOLAR DOOR. "
    "THE SHINING STAR FORMS IN ITS LIGHT.",
    {MOVE, LOOK, {"SHIFT", "L3", "RUN (RUNNING JUMP)"}, {"SPACE", "A", "JUMP / SWIM UP"}, {"E (HOLD)", "X", "GRAB LEDGE / PUSH"},
     {"E", "X", "LEVER / SAVE CRYSTAL"}, {"C", "B", "LET GO / DIVE"}, {"L-MOUSE", "RT", "PISTOLS (AUTO-AIM)"}, HELP, LEAVE},
    10,
    {"STANDING JUMP: ONE TILE. RUNNING JUMP: TWO.", "HOLD E IN THE AIR TO CATCH A LEDGE, THEN JUMP TO PULL UP.",
     "E + FORWARD PUSHES A BLOCK, E + BACK PULLS IT.", "FEWER SAVES AND ALL THREE JADE EARN SNOWFLAKES."},
    4,
};

static const cadv_help k_range = {
    "PRACTICE RANGE",
    "THIS PORTAL'S GAME IS STILL THAWING. UNTIL IT SHIPS, THE PRACTICE RANGE STANDS IN: DEFEAT THE "
    "IMPS, TAKE THE STAR AND RETURN TO THE HALL.",
    {MOVE, LOOK, {"L-MOUSE", "RT", "FIRE"}, {"SPACE", "A", "JUMP"}, {"E", "X", "USE"}, {"1 - 6", "LB / RB", "WEAPONS"}, HELP, LEAVE},
    8,
    {"ONLY THE DEVELOPMENT OVERRIDE OPENS THIS RANGE."},
    1,
};

const cadv_help *cadv_help_get(int day, bool implemented)
{
    if (day <= 0) {
        return &k_hall;
    }
    if (!implemented) {
        return &k_range;
    }
    switch (day) {
    case 1:
        return &k_day1;
    case 2:
        return &k_day2;
    case 3:
        return &k_day3;
    case 4:
        return &k_day4;
    default:
        return &k_range; /* GCOVR_EXCL_LINE: every shipped day has its own page */
    }
}

const char *cadv_help_key(cadv_device_kind device)
{
    return device == CADV_DEVICE_GAMEPAD ? "BACK" : device == CADV_DEVICE_TOUCH ? "?" : "F1";
}

int cadv_help_wrap(const char *text, int width, char lines[][64], int max_lines)
{
    int n = 0;
    int len = 0;
    const char *p = text;
    if (width > 63) {
        width = 63;
    }
    if (max_lines <= 0 || width <= 0) {
        return 0;
    }
    lines[0][0] = '\0';
    while (*p) {
        const char *w = p;
        int wl = 0;
        while (*p == ' ') {
            ++p;
        }
        w = p;
        while (p[wl] && p[wl] != ' ') {
            ++wl;
        }
        if (wl == 0) {
            break;
        }
        if (wl > width) {
            wl = width; /* a word longer than the line is cut */
        }
        if (len > 0 && len + 1 + wl > width) {
            if (n + 1 >= max_lines) {
                return n + 1;
            }
            ++n;
            len = 0;
            lines[n][0] = '\0';
        }
        if (len > 0) {
            lines[n][len++] = ' ';
        }
        memcpy(&lines[n][len], w, (size_t)wl);
        len += wl;
        lines[n][len] = '\0';
        p = w;
        while (*p && *p != ' ') {
            ++p;
        }
    }
    return len > 0 || n > 0 ? n + 1 : 0;
}

/* ---- drawing -------------------------------------------------------------------------- */

static const char *key_for(const cadv_help_control *c, cadv_device_kind device)
{
    return device == CADV_DEVICE_GAMEPAD || device == CADV_DEVICE_TOUCH ? c->pad : c->keys;
}

static void chip(cadv_ui *ui, float x, float y, const char *text, cadv_vec4 bg, cadv_vec4 fg)
{
    float w = 6.0f * (float)strlen(text) + 5.0f;
    cadv_ui_rect(ui, x, y, w, 9, bg);
    cadv_ui_rect(ui, x, y + 8, w, 1, cadv_v4(bg.x * 0.6f, bg.y * 0.6f, bg.z * 0.6f, bg.w)); /* bevel */
    cadv_ui_text(ui, x + 3, y + 1, 1, fg, text);
}

/* Draws one tip as a bulleted, wrapped paragraph if it fits completely
 * between y and bottom; returns the new y, or y unchanged when it does not. */
static float tip(cadv_ui *ui, float x, float y, float bottom, float width, const char *text)
{
    char lines[8][64];
    int n = cadv_help_wrap(text, (int)((width - 10.0f) / 6.0f), lines, 8);
    int j;
    if (y + 9.0f * (float)n > bottom) {
        return y;
    }
    if (!ui) {
        return y + 9.0f * (float)n + 3.0f; /* measuring only */
    }
    cadv_ui_rect(ui, x + 1, y + 3, 2, 2, cadv_v4(1.0f, 0.85f, 0.4f, 1));
    for (j = 0; j < n; ++j) {
        cadv_ui_text(ui, x + 8, y, 1, cadv_v4(0.6f, 0.7f, 0.8f, 1), lines[j]);
        y += 9.0f;
    }
    return y + 3.0f;
}

static void heading(cadv_ui *ui, float x, float y, const char *text)
{
    cadv_ui_text(ui, x, y, 1, cadv_v4(0.55f, 0.9f, 1.0f, 1), text);
    cadv_ui_rect(ui, x, y + 8, 6.0f * (float)strlen(text), 1, cadv_v4(0.55f, 0.9f, 1.0f, 0.5f));
}

int cadv_help_draw(cadv_ui *ui, const cadv_help *h, cadv_device_kind device, float open, int ticks, int page)
{
    cadv_vec4 frost = cadv_v4(0.55f, 0.9f, 1.0f, 1);
    cadv_vec4 gold = cadv_v4(1.0f, 0.85f, 0.4f, 1);
    cadv_vec4 text = cadv_v4(0.88f, 0.92f, 1.0f, 1);
    cadv_vec4 dim = cadv_v4(0.6f, 0.7f, 0.8f, 1);
    float k = cadv_clampf(open, 0.0f, 1.0f);
    float ease = k * k * (3.0f - 2.0f * k);
    float m = (float)ui->width > 360.0f ? 14.0f : 6.0f;
    float pw = (float)ui->width - 2.0f * m;
    float ph = (float)ui->height - 2.0f * m;
    float px = m;
    float py = m + (1.0f - ease) * 24.0f;
    float bottom = py + ph - 19.0f;
    float lx = px + 6.0f;
    float leftw;
    float rx;
    float rightw;
    float ly;
    float ry;
    int keyw = 0;
    int actw = 0;
    int i;
    int first = 0;
    int rows;
    int pages = 1;
    char lines[14][64];
    char buf[96];
    int n;
    /* dim the game, then a frosted glass panel with a lit border */
    cadv_ui_rect(ui, 0, 0, (float)ui->width, (float)ui->height, cadv_v4(0.0f, 0.02f, 0.06f, 0.55f * ease));
    cadv_ui_rect(ui, px - 1, py - 1, pw + 2, ph + 2, cadv_v4(frost.x, frost.y, frost.z, 0.8f * ease));
    cadv_ui_rect(ui, px, py, pw, ph, cadv_v4(0.03f, 0.06f, 0.12f, 0.92f * ease));
    if (ease < 0.6f) {
        return 1; /* text appears once the panel has slid in */
    }
    /* header band with the title and a twinkling star */
    cadv_ui_rect(ui, px, py, pw, 20, cadv_v4(0.08f, 0.16f, 0.28f, 1));
    cadv_ui_rect(ui, px, py + 20, pw, 1, cadv_v4(frost.x, frost.y, frost.z, 0.6f));
    cadv_ui_text(ui, px + 6, py + 6, 1, (ticks / 20) % 2 ? gold : cadv_v4(1, 1, 0.8f, 1), "*");
    if (12.0f * (float)strlen(h->title) + 20.0f < pw - 40.0f) {
        cadv_ui_text(ui, px + 16, py + 3, 2, gold, h->title);
    } else {
        cadv_ui_text(ui, px + 16, py + 6, 1, gold, h->title);
    }
    cadv_ui_text(ui, px + pw - 6.0f * 4.0f - 6.0f, py + 6, 1, dim, "HELP");
    /* the controls column is as wide as it needs to be (40..58 % of the panel) */
    for (i = 0; i < h->control_count; ++i) {
        int kl = (int)strlen(key_for(&h->controls[i], device));
        int al = (int)strlen(h->controls[i].action);
        keyw = kl > keyw ? kl : keyw;
        actw = al > actw ? al : actw;
    }
    leftw = cadv_clampf(6.0f * (float)(keyw + actw) + 12.0f, pw * 0.40f, pw * 0.58f);
    rx = lx + leftw + 8.0f;
    rightw = px + pw - 6.0f - rx;
    /* small screens: controls that do not fit and the tips go to page 2 */
    rows = (int)((bottom - (py + 38.0f)) / 10.0f);
    if (rows < h->control_count) {
        pages = 2;
    }
    if (page > 0 && pages > 1) {
        first = rows;
    }
    /* left: controls */
    ly = py + 26.0f;
    heading(ui, lx, ly, first > 0 ? "MORE CONTROLS" : "CONTROLS");
    ly += 12.0f;
    for (i = first; i < h->control_count && ly + 9.0f <= bottom; ++i) {
        const cadv_help_control *c = &h->controls[i];
        float ax = lx + 6.0f * (float)keyw + 9.0f;
        int room = (int)((lx + leftw - ax) / 6.0f);
        chip(ui, lx, ly, key_for(c, device), cadv_v4(0.75f, 0.85f, 0.95f, 1), cadv_v4(0.05f, 0.08f, 0.15f, 1));
        snprintf(buf, sizeof buf, "%.*s", room > 0 ? room : 0, c->action);
        cadv_ui_text(ui, ax, ly + 1, 1, text, buf);
        ly += 10.0f;
    }
    /* right: the goal (page 1 only when there are two) */
    ry = py + 26.0f;
    if (first == 0) {
        heading(ui, rx, ry, "GOAL");
        ry += 12.0f;
        n = cadv_help_wrap(h->goal, (int)(rightw / 6.0f), lines, (int)((bottom - ry) / 9.0f) < 14 ? (int)((bottom - ry) / 9.0f) : 14);
        for (i = 0; i < n; ++i) {
            cadv_ui_text(ui, rx, ry, 1, text, lines[i]);
            ry += 9.0f;
        }
    }
    /* tips flow into whichever column has room, whole tips only */
    if (h->tip_count > 0 && (pages == 1 || first > 0)) {
        bool right = bottom - ry >= bottom - ly - 4.0f;
        float x = right ? rx : lx;
        float w = right ? rightw : leftw;
        float y = (right ? ry : ly) + (right && first > 0 ? 0.0f : 5.0f);
        if (tip(NULL, x, y + 12.0f, bottom, w, h->tips[0]) != y + 12.0f) {
            heading(ui, x, y, "TIPS");
            y += 12.0f;
            for (i = 0; i < h->tip_count; ++i) {
                float ny = tip(ui, x, y, bottom, w, h->tips[i]);
                if (ny == y && right && ly + 6.0f < bottom) {
                    /* out of room on the right: continue under the controls */
                    right = false;
                    x = lx;
                    w = leftw;
                    y = ly + 5.0f;
                    ny = tip(ui, x, y, bottom, w, h->tips[i]);
                }
                y = ny;
            }
        }
    }
    /* footer: how to close */
    cadv_ui_rect(ui, px, py + ph - 15.0f, pw, 1, cadv_v4(frost.x, frost.y, frost.z, 0.4f));
    if (pages > 1) {
        snprintf(buf, sizeof buf, "PAUSED  %d/2 %s: PAGE", first > 0 ? 2 : 1, device == CADV_DEVICE_KBM ? "WHEEL" : "LB/RB");
        cadv_ui_text(ui, px + 6, py + ph - 11.0f, 1, dim, buf);
    } else {
        cadv_ui_text(ui, px + 6, py + ph - 11.0f, 1, dim, "GAME PAUSED");
    }
    snprintf(buf, sizeof buf, "%s CLOSE", cadv_help_key(device));
    chip(ui, px + pw - 6.0f * (float)strlen(buf) - 12.0f, py + ph - 12.0f, buf, cadv_v4(0.55f, 0.9f, 1.0f, 1),
         cadv_v4(0.03f, 0.06f, 0.12f, 1));
    return pages;
}

void cadv_help_draw_hint(cadv_ui *ui, cadv_device_kind device, float alpha)
{
    char buf[24];
    float a = cadv_clampf(alpha, 0.0f, 1.0f);
    float y = (float)ui->height - 44.0f;
    snprintf(buf, sizeof buf, "%s", cadv_help_key(device));
    if (a <= 0.0f) {
        return;
    }
    cadv_ui_rect(ui, 3, y - 2, 6.0f * (float)strlen(buf) + 44.0f, 13, cadv_v4(0.03f, 0.06f, 0.12f, 0.7f * a));
    chip(ui, 5, y, buf, cadv_v4(0.55f, 0.9f, 1.0f, a), cadv_v4(0.03f, 0.06f, 0.12f, a));
    cadv_ui_text(ui, 6.0f * (float)strlen(buf) + 13.0f, y + 1, 1, cadv_v4(0.88f, 0.92f, 1.0f, a), "HELP");
}

void cadv_help_draw_touch(cadv_ui *ui, const cadv_input *input)
{
    int i;
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    if (!cadv_input_touch_visible(input)) {
        return;
    }
    for (i = 0; i < input->touch_button_count; ++i) {
        const cadv_touch_button *b = &input->touch_buttons[i];
        float r = b->r * (float)ui->height;
        float bx = b->x * (float)ui->width;
        float by = b->y * (float)ui->height;
        cadv_ui_rect(ui, bx - r, by - r, 2 * r, 2 * r, cadv_v4(1, 1, 1, b->down ? 0.35f : 0.18f));
        cadv_ui_text_centered(ui, bx, by - 3, 1, white, b->label);
    }
    for (i = 0; i < CADV_MAX_FINGERS; ++i) {
        const cadv_finger *f = &input->fingers[i];
        if (f->active && f->role == 0) {
            float x0 = f->x0 * (float)ui->width;
            float y0 = f->y0 * (float)ui->height;
            cadv_ui_rect(ui, x0 - 16, y0 - 16, 32, 32, cadv_v4(1, 1, 1, 0.15f));
            cadv_ui_rect(ui, f->x * (float)ui->width - 5, f->y * (float)ui->height - 5, 10, 10, cadv_v4(1, 1, 1, 0.4f));
        }
    }
}
