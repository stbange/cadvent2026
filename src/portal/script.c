/* SPDX-License-Identifier: MIT */
#include "portal/script.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/mathx.h"

static const char *const k_actions[CADV_ACT_COUNT] = {
    "jump", "crouch", "fire", "alt", "use", "radial", "run", "pause",
    "next", "prev", "w1", "w2", "w3", "w4", "w5", "w6", "map"};

int cadv_script_action_from_name(const char *name)
{
    int i;
    for (i = 0; i < CADV_ACT_COUNT; ++i) {
        if (strcmp(name, k_actions[i]) == 0) {
            return i;
        }
    }
    return -1;
}

bool cadv_script_compare(double lhs, cadv_cmp cmp, double rhs)
{
    const double eps = 1e-6;
    switch (cmp) {
    case CADV_CMP_EQ: return lhs > rhs - eps && lhs < rhs + eps;
    case CADV_CMP_NE: return !(lhs > rhs - eps && lhs < rhs + eps);
    case CADV_CMP_LT: return lhs < rhs;
    case CADV_CMP_LE: return lhs <= rhs + eps;
    case CADV_CMP_GT: return lhs > rhs;
    default: return lhs >= rhs - eps;
    }
}

static bool parse_cmp(const char *s, cadv_cmp *out)
{
    static const char *const ops[6] = {"==", "!=", "<", "<=", ">", ">="};
    int i;
    for (i = 0; i < 6; ++i) {
        if (strcmp(s, ops[i]) == 0) {
            *out = (cadv_cmp)i;
            return true;
        }
    }
    return false;
}

static bool fail(cadv_script *s, int line, const char *what)
{
    snprintf(s->error, sizeof s->error, "line %d: %s", line, what);
    return false;
}

static bool parse_line(cadv_script *s, char *line, int lineno)
{
    char w[6][256];
    int n;
    cadv_script_cmd c;
    memset(&c, 0, sizeof c);
    memset(w, 0, sizeof w);
    c.line = lineno;
    n = sscanf(line, "%255s %255s %255s %255s %255s %255s", w[0], w[1], w[2], w[3], w[4], w[5]);
    if (n <= 0 || w[0][0] == '#') {
        return true;
    }
    if (s->count >= CADV_SCRIPT_MAX_CMDS) {
        return fail(s, lineno, "too many commands");
    }
    if (strcmp(w[0], "wait") == 0 && n == 2) {
        c.op = CADV_SOP_WAIT;
        c.ticks = atoi(w[1]);
    } else if ((strcmp(w[0], "hold") == 0 && n == 3) || (strcmp(w[0], "press") == 0 && n == 2)) {
        c.op = w[0][0] == 'h' ? CADV_SOP_HOLD : CADV_SOP_PRESS;
        c.action = cadv_script_action_from_name(w[1]);
        c.ticks = c.op == CADV_SOP_HOLD ? atoi(w[2]) : 1;
        if (c.action < 0) {
            return fail(s, lineno, "unknown action");
        }
    } else if (strcmp(w[0], "move") == 0 && n == 4) {
        c.op = CADV_SOP_MOVE;
        c.a = (float)atof(w[1]);
        c.b = (float)atof(w[2]);
        c.ticks = atoi(w[3]);
    } else if (strcmp(w[0], "turn") == 0 && n == 3) {
        c.op = CADV_SOP_TURN;
        c.a = (float)atof(w[1]);
        c.b = (float)atof(w[2]);
    } else if (strcmp(w[0], "face") == 0 && n == 2) {
        c.op = CADV_SOP_FACE;
        c.a = (float)atof(w[1]);
    } else if (strcmp(w[0], "teleport") == 0 && n == 5) {
        c.op = CADV_SOP_TELEPORT;
        c.a = (float)atof(w[1]);
        c.b = (float)atof(w[2]);
        c.c = (float)atof(w[3]);
        c.d = (float)atof(w[4]);
    } else if (strcmp(w[0], "walk_to") == 0 && n == 4) {
        c.op = CADV_SOP_WALK_TO;
        c.a = (float)atof(w[1]);
        c.b = (float)atof(w[2]);
        c.ticks = atoi(w[3]);
    } else if (strcmp(w[0], "bot") == 0 && n == 2) {
        c.op = CADV_SOP_BOT;
        c.ticks = atoi(w[1]);
    } else if ((strcmp(w[0], "expect") == 0 && n == 4) || (strcmp(w[0], "wait_until") == 0 && n == 5)) {
        c.op = w[0][0] == 'e' ? CADV_SOP_EXPECT : CADV_SOP_WAIT_UNTIL;
        snprintf(c.text, sizeof c.text, "%s", w[1]);
        if (!parse_cmp(w[2], &c.cmp)) {
            return fail(s, lineno, "unknown comparison");
        }
        c.a = (float)atof(w[3]);
        c.ticks = c.op == CADV_SOP_WAIT_UNTIL ? atoi(w[4]) : 0;
    } else if (strcmp(w[0], "expect_event") == 0 && n == 2) {
        c.op = CADV_SOP_EXPECT_EVENT;
        snprintf(c.text, sizeof c.text, "%s", w[1]);
    } else if (strcmp(w[0], "screenshot") == 0 && n == 2) {
        c.op = CADV_SOP_SCREENSHOT;
        snprintf(c.text, sizeof c.text, "%s", w[1]);
    } else if (strcmp(w[0], "print") == 0 && n == 2) {
        c.op = CADV_SOP_PRINT;
        snprintf(c.text, sizeof c.text, "%s", w[1]);
    } else if (strcmp(w[0], "log") == 0 && n >= 2) {
        c.op = CADV_SOP_LOG;
        snprintf(c.text, sizeof c.text, "%s", line + (strstr(line, "log") - line) + 4);
    } else {
        return fail(s, lineno, "unknown command or wrong argument count");
    }
    s->cmd[s->count++] = c;
    return true;
}

bool cadv_script_parse(cadv_script *s, const char *text)
{
    char line[512];
    int lineno = 0;
    memset(s, 0, sizeof *s);
    while (*text) {
        const char *nl = strchr(text, '\n');
        size_t len = nl ? (size_t)(nl - text) : strlen(text);
        ++lineno;
        if (len >= sizeof line) {
            return fail(s, lineno, "line too long");
        }
        memcpy(line, text, len);
        line[len] = '\0';
        if (len > 0 && line[len - 1] == '\r') {
            line[len - 1] = '\0';
        }
        if (!parse_line(s, line, lineno)) {
            return false;
        }
        text += len;
        if (*text == '\n') {
            ++text;
        }
    }
    return true;
}

bool cadv_script_load(cadv_script *s, const char *path)
{
    FILE *f = fopen(path, "rb");
    static char buf[65536];
    size_t n;
    if (!f) {
        memset(s, 0, sizeof *s);
        snprintf(s->error, sizeof s->error, "cannot open %s", path);
        return false;
    }
    n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = '\0';
    return cadv_script_parse(s, buf);
}

static void next(cadv_script *s)
{
    ++s->pc;
    s->elapsed = 0;
}

static bool failed(cadv_script *s, const cadv_script_cmd *c, const char *what)
{
    snprintf(s->error, sizeof s->error, "line %d: %s", c->line, what);
    s->failed = true;
    s->done = true;
    return false;
}

static double q(const cadv_script_host *h, const char *key, bool *ok)
{
    double v = 0.0;
    *ok = h->query && h->query(h->user, key, &v);
    return v;
}

bool cadv_script_step(cadv_script *s, const cadv_script_host *h, cadv_input_frame *out)
{
    memset(out, 0, sizeof *out);
    for (;;) {
        cadv_script_cmd *c;
        bool ok;
        double v;
        char msg[128];
        if (s->done) {
            return false;
        }
        if (s->pc >= s->count) {
            s->done = true;
            return false;
        }
        c = &s->cmd[s->pc];
        switch (c->op) {
        case CADV_SOP_WAIT:
            if (s->elapsed++ < c->ticks) {
                return true;
            }
            next(s);
            continue;
        case CADV_SOP_HOLD:
        case CADV_SOP_PRESS:
            if (s->elapsed < c->ticks) {
                out->held = CADV_ACT_BIT(c->action);
                out->pressed = s->elapsed == 0 ? out->held : 0;
                ++s->elapsed;
                return true;
            }
            next(s);
            continue;
        case CADV_SOP_MOVE:
            if (s->elapsed++ < c->ticks) {
                out->move_x = c->a;
                out->move_y = c->b;
                return true;
            }
            next(s);
            continue;
        case CADV_SOP_TURN:
            out->look_x = c->a;
            out->look_y = c->b;
            next(s);
            return true;
        case CADV_SOP_FACE:
            v = q(h, "player.yaw", &ok);
            if (!ok) {
                return failed(s, c, "face: player.yaw unavailable");
            }
            out->look_x = cadv_wrap_angle(c->a - (float)v);
            next(s);
            return true;
        case CADV_SOP_TELEPORT:
            if (h->teleport) {
                h->teleport(h->user, c->a, c->b, c->c, c->d);
            }
            next(s);
            continue;
        case CADV_SOP_WALK_TO: {
            double px = q(h, "player.x", &ok);
            double pz;
            double yaw;
            bool ok2;
            bool ok3;
            float dx;
            float dz;
            float dy;
            pz = q(h, "player.z", &ok2);
            yaw = q(h, "player.yaw", &ok3);
            if (!ok || !ok2 || !ok3) {
                return failed(s, c, "walk_to: player position unavailable");
            }
            dx = c->a - (float)px;
            dz = c->b - (float)pz;
            if (dx * dx + dz * dz < 0.36f) {
                next(s);
                continue;
            }
            if (s->elapsed++ >= c->ticks) {
                return failed(s, c, "walk_to: timed out");
            }
            dy = cadv_wrap_angle(cadv_atan2f(-dx, -dz) - (float)yaw);
            out->look_x = cadv_clampf(dy, -0.2f, 0.2f);
            out->move_y = cadv_absf(dy) < 0.5f ? 1.0f : 0.2f;
            return true;
        }
        case CADV_SOP_BOT: {
            bool mode_ok;
            double mode = q(h, "mode", &mode_ok);
            if (s->elapsed == 0) {
                c->b = mode_ok ? (float)mode : -1.0f; /* remember where the bot started */
            }
            v = q(h, "outcome", &ok);
            if ((ok && v != 0.0) || (s->elapsed > 0 && mode_ok && (float)mode != c->b)) {
                next(s);
                continue;
            }
            if (s->elapsed++ >= c->ticks) {
                bool okx;
                bool okz;
                bool okr;
                double px = q(h, "player.x", &okx);
                double pz = q(h, "player.z", &okz);
                double rt = q(h, "route", &okr);
                snprintf(msg, sizeof msg, "bot: did not finish in time (player at %.1f %.1f, route %g)",
                         okx ? px : 0.0, okz ? pz : 0.0, okr ? rt : -1.0);
                return failed(s, c, msg);
            }
            if (h->bot) {
                h->bot(h->user, out);
            }
            return true;
        }
        case CADV_SOP_EXPECT:
        case CADV_SOP_WAIT_UNTIL:
            v = q(h, c->text, &ok);
            if (!ok) {
                snprintf(msg, sizeof msg, "unknown key '%.80s'", c->text);
                return failed(s, c, msg);
            }
            if (cadv_script_compare(v, c->cmp, c->a)) {
                next(s);
                continue;
            }
            if (c->op == CADV_SOP_WAIT_UNTIL && s->elapsed++ < c->ticks) {
                return true;
            }
            snprintf(msg, sizeof msg, "expected %.60s (%g) to be %s %g", c->text, v,
                     c->cmp == CADV_CMP_EQ ? "==" : c->cmp == CADV_CMP_NE ? "!=" :
                     c->cmp == CADV_CMP_LT ? "<" : c->cmp == CADV_CMP_LE ? "<=" :
                     c->cmp == CADV_CMP_GT ? ">" : ">=", (double)c->a);
            return failed(s, c, msg);
        case CADV_SOP_EXPECT_EVENT:
            if (!h->event || !h->event(h->user, c->text)) {
                snprintf(msg, sizeof msg, "event '%.80s' did not happen", c->text);
                return failed(s, c, msg);
            }
            next(s);
            continue;
        case CADV_SOP_SCREENSHOT:
            if (h->screenshot && !h->screenshot(h->user, c->text)) {
                return failed(s, c, "screenshot failed");
            }
            next(s);
            return true; /* end the tick so the frame is rendered before going on */
        case CADV_SOP_PRINT:
            v = q(h, c->text, &ok);
            snprintf(msg, sizeof msg, "%.80s = %g%s", c->text, v, ok ? "" : " (unknown)");
            if (h->log) {
                h->log(h->user, msg);
            }
            next(s);
            continue;
        default:
            if (h->log) {
                h->log(h->user, c->text);
            }
            next(s);
            continue;
        }
    }
}
