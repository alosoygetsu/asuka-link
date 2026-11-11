/* The home screen once the disc is read: Asuka City at dusk, a bar on top with the pages, the title, chips and Play. */
#include "launcher.h"

#ifndef LAUNCHER_VERSION
#define LAUNCHER_VERSION "dev"
#endif

#define GAME_NAME "Digimon World 2003 (Europe)"

enum {
    BAR_H = 28, MARGIN = 16, NAV_X = 160, NAV_PAD = 3, NAV_END_MAX = 444, TITLE_X = 38, TITLE_Y = 38, SUB_Y = 66,
    CHIPS_Y = 86, CHIP_H = 14, CHIP_GAP = 6, BTN_Y = 108, BTN_H = 34, BTN_GAP = 10, SCENE_BASE = 200,
    TEAM_Y = 152, CARD_Y = 166, CARD_W = 144, CARD_H = 30, CARD_GAP = 8
};

static bool inside(SDL_Rect r, int x, int y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

/* ---- where things are, for the language given: drawing, the mouse and --check use the same numbers ---- */

/* the pages, left to right */
static const short PAGES[4] = {S_PLAY, S_SETTINGS, S_SAVES, S_TOOLS};

static SDL_Rect nav_rect(const Lang *l, int id)
{
    int x = NAV_X;
    for (int i = 0; PAGES[i] != id; i++) {
        x += text_width(l->s[PAGES[i]], 1) + 2 * NAV_PAD + 1;
    }
    return (SDL_Rect){x, 4, text_width(l->s[id], 1) + 2 * NAV_PAD, BAR_H - 8};
}

static void chip_text(const Lang *l, int i, const char *tag, char *out, int size)
{
    if (i == 0) {
        SDL_snprintf(out, size, "%s SLES-03936", l->s[S_DISC_OK]);
    } else if (i == 1) {
        SDL_snprintf(out, size, "%s", LAUNCHER_VERSION);
    } else {
        SDL_snprintf(out, size, "%s %s", l->s[S_RELEASE], tag);
    }
}

static SDL_Rect chip_rect(const Lang *l, int i, const char *tag)
{
    char s[96];
    int x = MARGIN;
    for (int k = 0; k < i; k++) {
        chip_text(l, k, tag, s, sizeof s);
        x += text_width(s, 1) + 12 + CHIP_GAP;
    }
    chip_text(l, i, tag, s, sizeof s);
    return (SDL_Rect){x, CHIPS_Y, text_width(s, 1) + 12, CHIP_H};
}

static SDL_Rect play_rect(const Lang *l) { return (SDL_Rect){MARGIN, BTN_Y, SDL_max(104, text_width(l->s[S_PLAY], 2) + 52), BTN_H}; }

static SDL_Rect quit_rect(const Lang *l)
{
    const SDL_Rect p = play_rect(l);
    return (SDL_Rect){p.x + p.w + BTN_GAP, BTN_Y, text_width(l->s[S_QUIT], 1) + 28, BTN_H};
}

static SDL_Rect rect_of(const Lang *l, int id, const char *tag)
{
    switch (id) {
    case S_PLAY: return play_rect(l);
    case S_QUIT: return quit_rect(l);
    case S_SETTINGS:
    case S_SAVES:
    case S_TOOLS: return nav_rect(l, id);
    case S_RELEASE: return chip_rect(l, 2, tag);
    default: return (SDL_Rect){0, 0, 0, 0};
    }
}

static void team_info(const Lang *l, const Team *t, char *out, int size)
{
    SDL_snprintf(out, size, "%s %s  -  %d:%02d", l->s[S_LAST_SAVE], t->when, t->hours, t->minutes);
}

int bar_hit(int x, int y)
{
    for (int i = 0; i < 4; i++) {
        if (inside(nav_rect(L, PAGES[i]), x, y)) {
            return PAGES[i];
        }
    }
    return -1;
}

bool home_fits(const Lang *l)
{
    const Team worst = {.when = "2026-10-28 13:23", .hours = 999, .minutes = 59};
    char info[96];
    team_info(l, &worst, info, sizeof info);
    const SDL_Rect tools = nav_rect(l, S_TOOLS), chip = chip_rect(l, 2, "v99.99.99"), quit = quit_rect(l);
    return tools.x + tools.w <= NAV_END_MAX && text_width(l->s[S_READY], 2) <= 340 && text_width(l->s[S_TAGLINE], 1) <= NAV_X - TITLE_X - 2 &&
           chip.x + chip.w <= SCREEN_W - MARGIN && quit.x + quit.w <= SCREEN_W - MARGIN &&
           text_width(l->s[S_TEAM], 1) + text_width(info, 1) + 24 <= SCREEN_W - 2 * MARGIN && text_width(l->s[S_NO_TEAM], 1) <= SCREEN_W - 2 * MARGIN;
}

int home_hit(int x, int y)
{
    const Row *r = rows();
    for (int i = 0; i < list_rows(r); i++) {
        if (r[i].kind == R_ACTION && inside(rect_of(L, r[i].id, release_tag() ? release_tag() : ""), x, y)) {
            return i;
        }
    }
    return -1;
}

/* ---- the scene ---- */

static void disc(int cx, int cy, int r, Uint32 colour, Uint8 alpha)
{
    for (int dy = -r; dy <= r; dy++) {
        const int w = (int)SDL_sqrtf((float)(r * r - dy * dy));
        fill_alpha(cx - w, cy + dy, 2 * w + 1, 1, colour, alpha);
    }
}

/* Asuka City at dusk: a few stars, the sky going orange at the horizon, the sun setting behind two rows of towers with
   lit windows, and the link (the co-op) arcing between the antennas of the two tallest */
static Uint32 s_seed;

static int rnd(int n)
{
    s_seed = s_seed * 1103515245u + 12345u;
    return (int)((s_seed >> 16) % (Uint32)n);
}

static void towers(int near)
{
    for (int x = near ? -6 : 0; x < SCREEN_W;) {
        const int w = near ? 14 + rnd(18) : 10 + rnd(14), right = x * 100 / SCREEN_W; /* taller to the right */
        const int h = near ? 10 + rnd(16) + right * 2 / 10 : 18 + rnd(22) + right * 3 / 10, top = SCENE_BASE - h;
        fill(x, top, w, h, near ? C_CITY_NEAR : C_CITY_FAR);
        if (!near && rnd(3) == 0) {
            fill(x + w / 2, top - 4, 1, 4, C_CITY_FAR); /* a mast */
        }
        for (int wy = top + 3; wy < SCENE_BASE - 3; wy += 4) { /* its windows, some lit */
            for (int wx = x + 2; wx < x + w - 2; wx += 3) {
                if (rnd(near ? 9 : 6) == 0) {
                    fill_alpha(wx, wy, 1, 1, C_WINDOW, near ? 220 : 120);
                }
            }
        }
        x += w + (near ? rnd(4) : 1 + rnd(3));
    }
}

/* the tower at x, w wide, h tall, with an antenna; where its tip is */
static SDL_Point antenna(int x, int w, int h)
{
    const int top = SCENE_BASE - h;
    fill(x, top, w, h, C_CITY_NEAR);
    fill(x + 1, top - 3, w - 2, 3, C_CITY_NEAR);
    fill(x + w / 2, top - 16, 1, 13, C_CITY_NEAR);
    fill_alpha(x + w / 2 - 1, top - 17, 3, 3, C_LINK, 110);
    fill(x + w / 2, top - 16, 1, 1, C_LINK);
    for (int wy = top + 4; wy < SCENE_BASE - 3; wy += 5) {
        fill_alpha(x + 2, wy, w - 4, 1, C_WINDOW, (Uint8)(wy % 3 ? 60 : 150));
    }
    return (SDL_Point){x + w / 2, top - 16};
}

static void scene(void)
{
    s_seed = 2003;
    for (int i = 0; i < 30; i++) {
        fill(150 + rnd(SCREEN_W - 150), BAR_H + 4 + rnd(60), 1, 1, i % 4 ? C_FAINT : C_SOFT);
    }
    for (int y = 90; y < SCENE_BASE; y++) { /* the dusk: deep blue, violet, then the horizon's rose */
        const int t = (y - 90) * 256 / (SCENE_BASE - 90);
        fill_alpha(0, y, SCREEN_W, 1, t < 110 ? C_DUSK_HI : t < 200 ? C_DUSK : C_DUSK_LOW, (Uint8)(40 + t * 150 / 256));
    }
    const int sx = 368, sy = SCENE_BASE - 50;
    disc(sx, sy, 60, C_DUSK_LOW, 30);
    disc(sx, sy, 44, C_SUN, 34);
    disc(sx, sy, 30, C_SUN, 255);
    disc(sx - 4, sy - 5, 19, C_SUN_HI, 255);
    for (int y = sy + 3, gap = 1; y < sy + 31; y += 3 + gap / 2, gap++) { /* the stripes across its lower half */
        fill_alpha(sx - 31, y, 63, 1 + gap / 3, C_DUSK, 255);
    }
    towers(0);
    const SDL_Point a = antenna(286, 13, 74), b = antenna(446, 15, 92);
    towers(1);
    for (int i = 0; i <= 64; i++) { /* the link: an arc of dots from one antenna to the other */
        const float t = i / 64.0f, x = a.x + (b.x - a.x) * t, y = a.y + (b.y - a.y) * t - 34 * 4 * t * (1 - t);
        fill_alpha((int)x - 1, (int)y - 1, 3, 3, C_LINK, 40);
        if (i % 2 == 0) {
            fill((int)x, (int)y, 1, 1, C_LINK);
        }
        if (i == 21 || i == 43) { /* two packets on their way */
            fill((int)x - 1, (int)y - 1, 3, 3, C_TEXT);
        }
    }
}

/* ---- the screen ---- */

void bar_draw(int page, int cur)
{
    const Lang *l = L;
    fill_alpha(0, 0, SCREEN_W, BAR_H, C_BG, 235);
    fill(0, BAR_H - 1, SCREEN_W, 1, C_RULE);
    art_icon(3, 3);
    text(TITLE_X, 3, 1, C_GOLD, APP_NAME);
    text(TITLE_X, 14, 1, C_SOFT, l->s[S_TAGLINE]);
    for (int i = 0; i < 4; i++) {
        const SDL_Rect t = nav_rect(l, PAGES[i]);
        const bool sel = PAGES[i] == cur, here = PAGES[i] == page;
        if (sel) {
            fill(t.x, t.y, t.w, t.h, C_SELECT);
        }
        text(t.x + NAV_PAD, t.y + 5, 1, sel ? C_GOLD : here ? C_TEXT : C_SOFT, l->s[PAGES[i]]);
        if (here) {
            fill(t.x + 3, BAR_H - 5, t.w - 6, 2, C_GOLD);
        }
    }
}

static void draw_chips(const Lang *l, int cur)
{
    const char *tag = release_tag();
    for (int i = 0; i < (tag ? 3 : 2); i++) {
        char s[96];
        const SDL_Rect c = chip_rect(l, i, tag ? tag : "");
        const bool news = i == 2, sel = news && cur == S_RELEASE;
        chip_text(l, i, tag ? tag : "", s, sizeof s);
        fill_alpha(c.x, c.y, c.w, c.h, sel ? C_SELECT : C_PANEL, 225);
        outline(c.x, c.y, c.w, c.h, news ? C_GOLD : C_EDGE);
        text(c.x + 6, c.y + 1, 1, news ? C_GOLD : C_TEXT, s);
    }
}

static void draw_buttons(const Lang *l, int cur)
{
    const SDL_Rect p = play_rect(l), q = quit_rect(l);
    const bool play = cur == S_PLAY, quit = cur == S_QUIT;
    fill_alpha(p.x, p.y + 3, p.w, p.h, 0x000000, 120);
    fill(p.x, p.y, p.w, p.h, play ? C_GOLD_HI : C_GOLD);
    fill_alpha(p.x, p.y + p.h - 3, p.w, 3, 0x000000, 50);
    if (play) {
        outline(p.x, p.y, p.w, p.h, C_TEXT);
    }
    triangle(p.x + 14, p.y + BTN_H / 2, 6, 1, C_BG);
    text(p.x + 34, p.y + (BTN_H - 2 * LINE_H) / 2 + 1, 2, C_BG, l->s[S_PLAY]);
    fill_alpha(q.x, q.y, q.w, q.h, quit ? C_SELECT : C_PANEL, 225);
    outline(q.x, q.y, q.w, q.h, quit ? C_GOLD : C_EDGE);
    text(q.x + 14, q.y + (BTN_H - LINE_H) / 2, 1, quit ? C_GOLD : C_TEXT, l->s[S_QUIT]);
}

/* the party, as cards: a coloured square with the partner's initial, the name and the level */
static void draw_team(const Lang *l)
{
    const Team *t = team_get();
    const int head = text_width(l->s[S_TEAM], 1);
    char info[96];
    text(MARGIN, TEAM_Y, 1, C_GOLD, l->s[S_TEAM]);
    if (!t) {
        fill(MARGIN + head + 8, TEAM_Y + 8, SCREEN_W - 2 * MARGIN - head - 8, 1, C_RULE);
        text(MARGIN, CARD_Y + 9, 1, C_SOFT, l->s[S_NO_TEAM]);
        return;
    }
    team_info(l, t, info, sizeof info);
    text_right(SCREEN_W - MARGIN, TEAM_Y, C_SOFT, info);
    fill(MARGIN + head + 8, TEAM_Y + 8, SCREEN_W - MARGIN - text_width(info, 1) - 8 - (MARGIN + head + 8) - 8, 1, C_RULE);
    for (int i = 0; i < t->count; i++) {
        const int x = MARGIN + i * (CARD_W + CARD_GAP);
        char level[32], initial[2] = {team_name(t->partner[i])[0], 0};
        fill_alpha(x, CARD_Y, CARD_W, CARD_H, C_PANEL, 225);
        outline(x, CARD_Y, CARD_W, CARD_H, C_EDGE);
        fill(x + 4, CARD_Y + 4, 22, 22, team_colour(t->partner[i]));
        text(x + 4 + (22 - text_width(initial, 1)) / 2, CARD_Y + 9, 1, C_BG, initial);
        SDL_snprintf(level, sizeof level, "%s %d", l->s[S_LEVEL], t->level[i]);
        text(x + 32, CARD_Y + 3, 1, C_TEXT, team_name(t->partner[i]));
        text(x + 32, CARD_Y + 15, 1, C_SOFT, level);
    }
}

bool home_on_bar(void)
{
    const Row *r = &rows()[g_app.sel[SC_HOME]];
    return r->kind == R_LANGUAGE || (r->kind == R_ACTION && (r->id == S_SETTINGS || r->id == S_SAVES || r->id == S_TOOLS || (r->id == S_PLAY && g_app.playtab)));
}

void home_draw(void)
{
    const Lang *l = L;
    const Row *r = &rows()[g_app.sel[SC_HOME]];
    const int cur = r->kind == R_ACTION ? r->id : -1;
    const bool bar = home_on_bar();
    scene();
    bar_draw(S_PLAY, bar ? cur : -1);
    text(MARGIN, TITLE_Y, 2, C_TEXT, l->s[S_READY]);
    text(MARGIN, SUB_Y, 1, C_SOFT, GAME_NAME);
    draw_chips(l, bar ? -1 : cur);
    draw_buttons(l, bar ? -1 : cur);
    draw_team(l);
}
